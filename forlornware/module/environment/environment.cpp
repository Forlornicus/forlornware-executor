#include "environment.hpp"

namespace forlorn_ware
{
	namespace yielder
	{
		using yield_return = std::function<int(lua_State* L)>;

		struct task_data
		{
			lua_State* state;
			std::function<yield_return()> generator;
			PTP_WORK work;
		};

		inline VOID CALLBACK thread_worker(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work)
		{
			auto* data = static_cast<task_data*>(context);

			try
			{
				auto yield_result = data->generator();
				int result_count = yield_result(data->state);

				lua_State* thread_ctx = lua_newthread(data->state);

				lua_getglobal(thread_ctx, "task");
				lua_getfield(thread_ctx, -1, "defer");
				lua_pushthread(data->state);
				lua_xmove(data->state, thread_ctx, 1);
				lua_pop(data->state, 1);

				for (int i = result_count; i >= 1; --i)
				{
					lua_pushvalue(data->state, -i);
					lua_xmove(data->state, thread_ctx, 1);
				}

				lua_pcall(thread_ctx, result_count + 1, 0, 0);
				lua_settop(thread_ctx, 0);
			}
			catch (const std::exception&)
			{
				// handle exceptions if needed
			}

			CloseThreadpoolWork(data->work);
			delete data;
		}

		inline int yield_execution(lua_State* L, const std::function<yield_return()>& generator)
		{
			lua_pushthread(L);
			lua_ref(L, -1);
			lua_pop(L, 1);

			auto* task = new task_data{ L, generator, nullptr };

			task->work = CreateThreadpoolWork(thread_worker, task, nullptr);
			if (task->work)
			{
				SubmitThreadpoolWork(task->work);
			}
			else
			{
				delete task;
			}

			L->base = L->top;
			L->status = LUA_YIELD;
			L->ci->flags |= 1;
			return -1;
		}
	}

	namespace env
	{
		namespace http
		{
			int get_objects(lua_State* L)
			{
				lua_getglobal(L, "game");
				lua_getfield(L, -1, "GetService");
				lua_pushvalue(L, -2);
				lua_pushstring(L, "InsertService");
				lua_call(L, 2, 1);

				lua_getfield(L, -1, "LoadLocalAsset");
				lua_pushvalue(L, -2);
				lua_pushstring(L, lua_tostring(L, 2));
				lua_call(L, 2, 1);

				lua_newtable(L);
				lua_pushvalue(L, -2);
				lua_rawseti(L, -2, 1);
				return 1;
			}

			int httpget(lua_State* L)
			{
				std::string url;
				if (!lua_isstring(L, 1)) {
					luaL_checkstring(L, 2);
					url = lua_tostring(L, 2);
				}
				else {
					url = lua_tostring(L, 1);
				}

				if (url.find("http://") != 0 && url.find("https://") != 0) {
					luaL_argerror(L, 1, "Invalid protocol (expected 'http://' or 'https://')");
				}

				std::optional<std::string> job_id;
				lua_getglobal(L, "game");
				lua_getfield(L, -1, "JobId");
				if (lua_isstring(L, -1))
					job_id = lua_tostring(L, -1);
				lua_pop(L, 2);

				uintptr_t place_id = 0;
				lua_getglobal(L, "game");
				lua_getfield(L, -1, "PlaceId");
				if (lua_isnumber(L, -1))
					place_id = static_cast<uintptr_t>(lua_tointeger(L, -1));
				lua_pop(L, 2);

				return forlorn_ware::yielder::yield_execution(L, [url, job_id, place_id]() -> forlorn_ware::yielder::yield_return {
					return [url, job_id, place_id](lua_State* L) -> int {
						std::string session_id_header;
						if (job_id.has_value()) {
							session_id_header = "{\"GameId\":\"" + job_id.value() + "\",\"PlaceId\":\"" + std::to_string(place_id) + "\"}";
						}
						else {
							session_id_header = "{\"GameId\":\"empty value\",\"PlaceId\":\"empty value\"}";
						}

						HINTERNET h_internet = InternetOpenA("Roblox/WinInet", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
						if (!h_internet) {
							lua_pushstring(L, "HttpGet failed: InternetOpenA failed");
							return 1;
						}

						HINTERNET h_url = InternetOpenUrlA(h_internet, url.c_str(), nullptr, 0, INTERNET_FLAG_RELOAD, 0);
						if (!h_url) {
							InternetCloseHandle(h_internet);
							lua_pushstring(L, "HttpGet failed: InternetOpenUrlA failed");
							return 1;
						}

						std::string response;
						char buffer[4096];
						DWORD bytes_read = 0;

						while (InternetReadFile(h_url, buffer, sizeof(buffer), &bytes_read) && bytes_read > 0)
							response.append(buffer, bytes_read);

						InternetCloseHandle(h_url);
						InternetCloseHandle(h_internet);

						if (response.empty()) {
							lua_pushstring(L, "HttpGet failed: empty response");
							return 1;
						}

						lua_pushlstring(L, response.data(), response.size());
						return 1;
						};
					});
			}
		}

		namespace test
		{
			int identify_executor(lua_State* L)
			{
				lua_pushstring(L, "ForlornWare");
				lua_pushstring(L, "1.0.0");
				return 2;
			}

			int get_genv(lua_State* L)
			{
				lua_pushvalue(L, LUA_ENVIRONINDEX);
				return 1;
			}
                        int set_clipboard(lua_State* L)
			{
				 luaL_checktype(L, 1, LUA_TSTRING);
                            std::string content = lua_tostring(L, 1);
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, content.size() + 1);
                                     memcpy(GlobalLock(hMem), content.data(), content.size());
                                     GlobalUnlock(hMem);
                                     OpenClipboard(0);
                                     EmptyClipboard();
                                    SetClipboardData(CF_TEXT, hMem);
                                    CloseClipboard();
                               return 0;
			}


			int load_string(lua_State* L)
			{
				luaL_checktype(L, 1, LUA_TSTRING);

				const char* source = lua_tostring(L, 1);
				const char* chunk_name = luaL_optstring(L, 2, "ForlornWare");

				std::string bytecode = compile_script(source);

				if (luau_load(L, chunk_name, bytecode.c_str(), bytecode.size(), 0) != LUA_OK)
				{
					lua_pushnil(L);
					lua_pushvalue(L, -2);
					return 2;
				}

				if (Closure* func = lua_toclosure(L, -1))
				{
					if (func->l.p)
						task_scheduler::set_proto_capabilities(func->l.p, &max_caps);
				}

				lua_setsafeenv(L, LUA_GLOBALSINDEX, false);
				return 1;
			}

			int get_hui(lua_State* L)
			{
				lua_getglobal(L, "game");
				lua_getfield(L, -1, "GetService");
				lua_pushvalue(L, -2);
				lua_pushstring(L, "CoreGui");
				lua_pcall(L, 2, 1, 0);
				lua_remove(L, -2);
				return 1;
			}

			struct scan_context
			{
				lua_State* thread;
				int result_index;
				int insert_index;
			};

			int get_scripts(lua_State* L)
			{
				scan_context ctx{ L, lua_gettop(L) + 1, 0 };
				lua_newtable(L);

				auto original_threshold = L->global->GCthreshold;
				L->global->GCthreshold = SIZE_MAX;

				luaM_visitgco(L, &ctx, [](void* userdata, lua_Page* page, GCObject* obj) -> bool {
					auto* sctx = static_cast<scan_context*>(userdata);

					if (isdead(sctx->thread->global, obj))
						return false;

					if (obj->gch.tt != LUA_TUSERDATA)
						return true;

					TValue* top = sctx->thread->top++;
					top->tt = LUA_TUSERDATA;
					top->value.p = reinterpret_cast<void*>(obj);

					if (strcmp(luaL_typename(sctx->thread, -1), "Instance") != 0) {
						lua_pop(sctx->thread, 1);
						return true;
					}

					lua_getfield(sctx->thread, -1, "ClassName");
					const char* classname = lua_tostring(sctx->thread, -1);

					bool is_script = classname &&
						(!strcmp(classname, "LocalScript") ||
							!strcmp(classname, "ModuleScript") ||
							!strcmp(classname, "CoreScript") ||
							!strcmp(classname, "Script"));

					if (is_script) {
						lua_pop(sctx->thread, 1);
						sctx->insert_index++;
						lua_rawseti(sctx->thread, sctx->result_index, sctx->insert_index);
					}
					else {
						lua_pop(sctx->thread, 2);
					}

					return true;
					});

				L->global->GCthreshold = original_threshold;
				return 1;
			}

			static const luaL_Reg functions[] = {
				{"identifyexecutor", identify_executor},
				{"httpget", http::httpget},

				{"getgenv", get_genv},
				{"gethui", get_hui},
				//{"getscripts", get_scripts}, // i will recode this later or whatever since it gets all corescripts by default
                                {"setclipboard", set_clipboard},
				{"loadstring", load_string},
				{nullptr, nullptr}
			};
		}

		namespace cache
		{
			void validate_instance(lua_State* L, int idx) 
			{
				const char* type_name = luaL_typename(L, idx);
				if (!type_name || strcmp(type_name, "Instance") != 0)
				{
					luaL_typeerrorL(L, idx, "Instance");
				}
			}

			int invalidate(lua_State* L)
			{
				luaL_checktype(L, 1, LUA_TUSERDATA);
				validate_instance(L, 1);

				void* instance_ptr = *static_cast<void**>(lua_touserdata(L, 1));

				lua_pushlightuserdata(L, (void*)roblox::push_instance);
				lua_gettable(L, LUA_REGISTRYINDEX);

				lua_pushlightuserdata(L, instance_ptr);
				lua_pushnil(L);
				lua_settable(L, -3);

				lua_pop(L, 1);
				return 0;
			}

			int replace(lua_State* L)
			{
				luaL_checktype(L, 1, LUA_TUSERDATA);
				luaL_checktype(L, 2, LUA_TUSERDATA);

				validate_instance(L, 1);
				validate_instance(L, 2);

				void* old_instance_ptr = *static_cast<void**>(lua_touserdata(L, 1));

				lua_pushlightuserdata(L, (void*)roblox::push_instance);
				lua_gettable(L, LUA_REGISTRYINDEX);

				lua_pushlightuserdata(L, old_instance_ptr);
				lua_pushvalue(L, 2);
				lua_settable(L, -3);

				lua_pop(L, 1);
				return 0;
			}

			int is_cached(lua_State* L)
			{
				luaL_checktype(L, 1, LUA_TUSERDATA);
				validate_instance(L, 1);

				void* instance_ptr = *static_cast<void**>(lua_touserdata(L, 1));

				lua_pushlightuserdata(L, (void*)roblox::push_instance);
				lua_gettable(L, LUA_REGISTRYINDEX);

				lua_pushlightuserdata(L, instance_ptr);
				lua_gettable(L, -2);

				bool cached = !lua_isnil(L, -1);
				lua_pop(L, 2);

				lua_pushboolean(L, cached);
				return 1;
			}

			int validate(lua_State* L)
			{
				luaL_checktype(L, 1, LUA_TUSERDATA);

				void** userdata = static_cast<void**>(lua_touserdata(L, 1));
				if (!userdata || !*userdata) {
					lua_pushboolean(L, 0);
					return 1;
				}

				void* raw_userdata = *userdata;

				lua_pushlightuserdata(L, (void*)roblox::push_instance);
				lua_rawget(L, LUA_REGISTRYINDEX);

				lua_pushlightuserdata(L, raw_userdata);
				lua_rawget(L, -2);

				bool already_cached = lua_type(L, -1) != LUA_TNIL;
				lua_pop(L, 1);

				if (!already_cached) {
					lua_pushlightuserdata(L, raw_userdata);
					lua_pushvalue(L, 1);
					lua_rawset(L, -3);
				}

				lua_pop(L, 1);

				lua_pushboolean(L, 1);
				return 1;
			}

			static const luaL_Reg functions[] = {
					{"invalidate", invalidate},
					{"validate", validate},
					{"replace", replace},
					{"iscached", is_cached},
					{nullptr, nullptr}
			};
		}
	}

	namespace name_call
	{
		static __int64 old_namecall;
		static __int64 old_index;

		auto namecall_hook(lua_State* L) -> int
		{
			const auto script_ptr = *(std::uintptr_t*)((std::uintptr_t)(L->userdata) + 0x50);

			if (L->namecall && !script_ptr)
			{
				const char* data = L->namecall->data;

				if (!strcmp(data, "HttpGet") || !strcmp(data, "HttpGetAsync"))
				{
					return env::http::httpget(L);
				}

				if (!strcmp(data, "GetObjects") || !strcmp(data, "GetObjectsAsync"))
				{
					return env::http::get_objects(L);
				}
			}

			return static_cast<int>(((__int64(__fastcall*)(__int64))old_namecall)((__int64)L));
		}

		auto index_hook(lua_State* L) -> int
		{
			auto state = (__int64)L;
			const auto script_ptr = *(std::uintptr_t*)((std::uintptr_t)(L->userdata) + 0x50);

			uintptr_t userdata = *(uintptr_t*)(state + 0x78);
			int level = static_cast<int>(*(uintptr_t*)(userdata + 0x30));

			if (lua_isstring(L, 2) && !script_ptr)
			{
				const char* data = luaL_checkstring(L, 2);

				if (!strcmp(data, "HttpGet") || !strcmp(data, "HttpGetAsync"))
				{
					lua_getglobal(L, "httpget");
					return 1;
				}

				if (!strcmp(data, "GetObjects") || !strcmp(data, "GetObjectsAsync"))
				{
					lua_getglobal(L, "getobjects");
					return 1;
				}
			}

			return static_cast<int>(((__int64(__fastcall*)(__int64))old_index)((__int64)L));
		}

		void initialize(lua_State* L)
		{
			lua_getglobal(L, "game");
			lua_getmetatable(L, -1);
			lua_getfield(L, -1, "__namecall");

			Closure* namecall = (Closure*)lua_topointer(L, -1);
			lua_CFunction namecall_f = namecall->c.f;
			old_namecall = (__int64)namecall_f;
			namecall->c.f = namecall_hook;

			lua_settop(L, 0);

			lua_getglobal(L, "game");
			lua_getmetatable(L, -1);
			lua_getfield(L, -1, "__index");

			Closure* index = (Closure*)lua_topointer(L, -1);
			lua_CFunction index_f = index->c.f;
			old_index = (__int64)index_f;
			index->c.f = index_hook;
		}
	}
}

void environment::initialize(lua_State* L)
{
    register_env_functions(L, forlorn_ware::env::test::functions);
	register_env_members(L, forlorn_ware::env::cache::functions, "cache");

	forlorn_ware::name_call::initialize(L);

    lua_newtable(L);
    lua_setglobal(L, "_G");

    lua_newtable(L);
    lua_setglobal(L, "shared");
}
