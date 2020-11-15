// handle.h - handles to C++ objects
#pragma once
#include <any>
#include <limits>
#include <memory>
#include <set>
#include <utility>
#include "excel.h"

// handle data type
using HANDLEX = double;
inline constexpr HANDLEX INVALID_HANDLEX = std::numeric_limits<HANDLEX>::quiet_NaN();

// handle argument types for add-ins
#define XLL_HANDLE XLL_DOUBLE
#define XLL_HANDLEX XLL_DOUBLE

namespace xll {

	/// <summary>
	/// Convert a pointer to a handle.
	/// </summary>
	/// <typeparam name="T">Any type</typeparam>
	/// <param name="p">A pointer to T</param>
	/// <returns>Returns a handle encoding the pointer bits.</returns>
	/// 
	template<class T>
	inline HANDLEX to_handle(T* p)
	{
		// first 16 bits of p are 0 and 2^(64 - 16) = 2^48
		// doubles exactly represent integers < 2^53
		return (HANDLEX)((uint64_t)p);
	}
	/// <summary>
	/// Convert a handle to a pointer.
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <param name="h">is a handle</param>
	/// <returns>Returns the decoded handle bits.</returns>
	/// 
	template<class T>
	inline T* to_pointer(HANDLEX h)
	{
		return (T*)((uint64_t)h);
	}

	/// <summary>
	/// HANDLEX defaulting to NaN. Shows up as #NUM! in Excel
	/// </summary>
	struct handlex {
		HANDLEX h;
		explicit handlex(double h = std::numeric_limits<double>::quiet_NaN())
			: h(h)
		{ }
		operator HANDLEX&()
		{
			return h;
		}
		operator const HANDLEX() const
		{
			return h;
		}
	};

	// compare underlying raw pointers
	template<class T>
	struct unique_ptrcmp {
		using is_transparent = void;

		template<class T>
		bool operator()(const std::unique_ptr<T>& a, const T* b) const
		{
			return a.get() < b;
		}

		template<class T>
		bool operator()(const T* a, const std::unique_ptr<T>& b) const
		{
			return a < b.get();
		}

		template<class T>
		bool operator()(const std::unique_ptr<T>& a, const std::unique_ptr<T>& b) const
		{
			return a.get() < b.get();
		}
	};

	/// <summary>
	/// Collection of handles parameterized by type.
	/// Use <c>handle<T> h(new T(...))</c> to create a handle.
	/// Functions that create handles must be uncalced.
	/// Use <c>handle<T> h_(h)</c> to lookup h returned by <c>get()</c>.
	/// Functions that use handles do not need to be uncalced.
	/// Unknown handles return null pointers.
	/// </summary>
	template<class T>
	class handle {
		// all active pointers of type T*
		inline static std::set<std::unique_ptr<T>, unique_ptrcmp<T>> ps;
		
		// Convert handle in caller to pointer.
		static T* caller()
		{
			// value in cell a function is called from
			OPER x = Excel(xlCoerce, Excel(xlfCaller));

			return x.xltype == xltypeNum ? to_pointer<T>(x.val.num) : nullptr;
		}

		// If calling cell has a known handle then delete corresponding object
		static void gc(void)
		{
			if (T* _p = caller()) {
				auto pi = ps.find(_p);
				// garbage collect
				if (pi != ps.end()) {
					ps.erase(pi); // calls delete
				}
			}
		}

		// underlying pointer
		T* p;
	public:
		/// <summary>
		/// Add a handle to the collection.
		/// </summary>
		handle(T* p) noexcept
			: p{ p }
		{
			ps.emplace(std::unique_ptr<T>(p));
			gc();
		}
		handle(HANDLEX h, bool check = true) noexcept
			: p(to_pointer<T>(h))
		{
			if (h and check) {
				if (ps.find(p) == ps.end()) {
					// unknown handle
					p = nullptr;
				}
			}
		}
		handle(const handle&) = default;
		handle& operator=(const handle&) = default;
		~handle()
		{ }

		operator bool() const
		{
			return p != nullptr;
		}

		// return value for Excel
		HANDLEX get() const
		{
			return to_handle(p);
		}
		// underlying pointer
		T* ptr() const
		{
			return p;
		}
		// act like a pointer
		T* operator->()
		{
			return p;
		}
		const T* operator->() const
		{
			return p;
		}
		// downcast to U
		template<class U> requires std::is_base_of_v<T,U>
		U* as()
		{
			return dynamic_cast<U*>(p);
		}
		// YAGNI???
		/*
		template<class U>
			requires std::is_base_of_v<T, U>
		const U* as() const
		{
			return dynamic_cast<U*>(p);
		}
		*/
	};

	// encode/decode???

}
