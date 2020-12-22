// oper.h - Wrapper for XLOPER
// Copyright (c) KALX, LLC. All rights reserved. No warranty made.
#pragma once
#include <algorithm>
#include <concepts>
#include <iostream>
#include <utility>
#include "utf8.h"
#include "xloper.h"
#include "sref.h"

#pragma warning(disable: 4996)

namespace xll {

	/// <summary>
	///  Wrapper for XLOPER/XLOPER12 structs.
	/// </summary>
	/// <typeparam name="XLOPERX">
	/// Either XLOPER or XLOPER12 
	/// </typeparam>
	/// An OPER corresponds to a cell or a 2-dimensional range of cells.
	/// It is a variant type that can be either a number, string, boolean,
	/// error, range, single reference, or integer. 
	template<class X>  
		requires (std::is_same_v<XLOPER, X> || std::is_same_v<XLOPER12, X>)
	class XOPER final : public X {
		typedef typename traits<X>::typex X_; // the _other_ type
		typedef typename traits<X>::xchar xchar;
		typedef typename traits<X_>::xchar charx;
		typedef typename traits<X>::xcstr xcstr;
		typedef typename traits<X_>::xcstr cstrx;
		typedef typename traits<X>::xint xint;
		typedef typename traits<X>::xrw xrw;
		typedef typename traits<X>::xcol xcol;
		typedef typename traits<X>::xint xint;
		typedef typename traits<X>::xref xref;

	public:
		using X::xltype;
		using X::val;

		void swap(XOPER& x) noexcept
		{
			using std::swap;

			swap(xltype, x.xltype);
			swap(val, x.val);
		}

		//
		// constructors
		//
		XOPER()
		{
			xltype = xltypeNil;
		}
		XOPER(const X& x)
		{
			if (x.xltype & xltypeScalar) {
				xltype = x.xltype;
				val = x.val;
			}
			else if (x.xltype & xltypeStr) {
				str_alloc(x.val.str + 1, x.val.str[0]);
			}
			else if (x.xltype & xltypeMulti) {
				multi_alloc(x.val.array.rows, x.val.array.columns);
				std::copy(x.val.array.lparray, x.val.array.lparray + size(), begin());
			}
			else {
				xltype = xltypeErr;
				val.err = xlerrValue;
			}
		}
		XOPER(const XOPER& o)
			: XOPER(static_cast<const X&>(o))
		{ }

		XOPER(XOPER&& o) noexcept
			: XOPER()
		{
			xltype = std::exchange(o.xltype, xltype);
			val = std::exchange(o.val, val);
		}
		//
		// Assignment
		//
		XOPER& operator=(const XOPER& o)
		{
			return *this = XOPER(o);
		}
		XOPER& operator=(XOPER&& o) noexcept
		{
			swap(o);

			return *this;
		}
		~XOPER()
		{
			oper_free();
		}

		// Does not involve memory needing to be freed.
		bool is_scalar() const
		{
			return xltype & xltypeScalar;
		}
		/**/
		explicit operator bool() const
		{
			switch (xltype & ~(xlbitDLLFree|xlbitXLFree)) {
			case xltypeNum:
				return val.num != 0;
				break;
			case xltypeStr:
				return val.str[0] != 0;
				break;
			case xltypeBool:
				return val.xbool != 0;
				break;
			case xltypeMulti:
				return std::any_of(begin(), end(), [](const auto& x) { return x; });
				break;
			case xltypeInt:
				return val.w != 0;
			}

			return false; // xltypeErr, xltypeMissig, xltypeNil
		}
		/**/
		// IEEE 64-bit floating point number
		explicit XOPER(double num)
		{
			xltype = xltypeNum;
			val.num = num;
		}
		XOPER operator=(double num)
		{
			oper_free();
			operator=(XOPER(num));

			return *this;
		}
		bool operator==(double num) const
		{
			return (xltype & xltypeNum) && val.num == num;
		}
		/* not working !!!
		// handy for using OPERs in numerical expressions
		operator double()
		{
			return (xltype & xltypeNum) ? val.num 
				 : (xltype & xltypeInt) ? val.w 
				 : (xltype & xltypeBool) ? val.xbool 
				 : std::numeric_limits<double>::quiet_NaN();
		}
		*/

		// xltypeStr given length
		XOPER(xcstr str, int n)
		{
			str_alloc(str, n);
		}
		// xltypeStr from NULL terminated string
		explicit XOPER(xcstr str)
			: XOPER(str, traits<X>::len(str))
		{
		}
		// xltypeStr from string literal
		template<size_t N>
		XOPER(xcstr(&str)[N])
			: XOPER(str, N)
		{
		}
		// convert to type appropriate for X
		explicit XOPER(cstrx str)
		{
			str_alloc(traits<X>::cvt(str), (size_t)-1);
		}
		// Construct from string literal
		template<size_t N>
		XOPER(cstrx (&str)[N])
		{
			str_alloc(traits<X>::cvt(str), (size_t)-1);
		}
		bool operator==(xcstr str) const
		{
			if (!(xltype & xltypeStr)) {
				return false;
			}

			size_t n = traits<X>::len(str);
			ensure(n < static_cast<size_t>(std::numeric_limits<xchar>::max()));
			
			if (val.str[0] != static_cast<xchar>(n))
				return false;

			return 0 == traits<X>::cmp(str, val.str + 1, n);
		}
		// no operator== for cstrx
		XOPER operator=(xcstr str)
		{
			oper_free();
			str_alloc(str, traits<X>::len(str));

			return *this;
		}
		XOPER operator=(cstrx str)
		{
			oper_free();
			str_alloc(traits<X>::cvt(str), (size_t)-1);

			return *this;
		}
		XOPER& append(const X& x)
		{
			ensure(x.xltype & xltypeStr);
			str_append(x.val.str + 1, x.val.str[0]);

			return *this;
		}
		XOPER& append(const X_& x)
		{
			ensure(x.xltype & xltypeStr);
			str_append(traits<X>::cvt(x.val.str + 1, x.val.str[0]), (size_t)-1);

			return *this;
		}
		// Like the Excel ampersand operator
		XOPER& operator&=(const X& x)
		{
			return append(x);
		}
		XOPER& operator&=(const X_& x)
		{
			return append(x);
		}
		XOPER& append(xcstr str = nullptr)
		{
			if (!str) {
				xchar null[1] = { 0 };
				str_append(null, 1);
			}
			else {
				str_append(str, traits<X>::len(str));
			}

			return *this;
		}
		XOPER& append(cstrx str)
		{
			str_append(traits<X>::cvt(str), (size_t)-1);

			return *this;
		}
		XOPER& operator&=(xcstr str)
		{
			return append(str);
		}
		XOPER& operator&=(cstrx str)
		{
			return append(str);
		}

		// xltypeBool
		explicit XOPER(bool xbool)
		{
			xltype = xltypeBool;
			val.xbool = xbool;
		}
		bool operator==(bool xbool) const
		{
			return (xltype & xltypeBool) && val.xbool == static_cast<typename traits<X>::xbool>(xbool);
		}
		XOPER operator=(bool xbool)
		{
			oper_free();
			operator=(XOPER(xbool));

			return *this;
		}

		// xltypeRef
		// xltypeErr - predifined as ErrXXX
		// xltypeFlow - not used

		// xltypeMulti
		XOPER(size_t rw, size_t col)
		{
			multi_alloc(rw, col);
		}
		// XOPER({XOPER(a), ...}). Use resize if needed.
		explicit XOPER(std::initializer_list<XOPER> x)
		{
			ensure(x.size() <= std::numeric_limits<xcol>::max());

			multi_alloc(1, static_cast<xcol>(x.size()));
			std::copy(x.begin(), x.end(), begin());
		}
		XOPER& resize(size_t rw, size_t col)
		{
			multi_realloc(rw, col);

			return *this;
		}
		size_t rows() const
		{
			return xll::rows(*this);
		}
		size_t columns() const
		{
			return xll::columns(*this);
		}
		size_t size() const
		{
			return xll::size(*this);
		}
		// STL friendly
		XOPER* begin()
		{
			return xll::begin(*this);
		}
		const XOPER* begin() const
		{
			return xll::begin(*this);
		}
		XOPER* end()
		{
			return xll::end(*this);
		}
		const XOPER* end() const
		{
			return xll::end(*this);
		}
		// one-dimensional index
		XOPER& operator[](size_t i)
		{
			return xll::index(*this, i);
		}
		const XOPER& operator[](size_t i) const
		{
			return xll::index(*this, i);
		}
		// two-dimensional index
		XOPER& operator()(size_t rw, size_t col)
		{
			return xll::index(*this, rw, col);
		}
		const XOPER& operator()(size_t rw, size_t col) const
		{
			return xll::index(*this, rw, col);
		}
		const XOPER& append_bottom(const X& x)
		{
			if (xltype == xltypeNil) {
				operator=(x);

				return *this;
			}
			// make a copy if memory overlap
			const XOPER& o = overlap(x) ? XOPER(x) : x;
			if (xltype == xltypeMulti) {
				ensure(columns() == xll::columns(x));
				multi_realloc(rows() + xll::rows(x), columns());
				std::copy(o.begin(), o.end(), end() - o.size());
			}
			else {
				XOPER tmp(1, 1);
				swap(tmp);
				operator[](0) = tmp;

				return append_bottom(o);
			}

			return *this;
		}
		// append_top
		// append_right
		// append_left

		// xltypeMissing - predefined as Missing
		// xltypeNil - predefined as Nil

		// xltypeSRef - reference to a single range
		XOPER(xrw row, xcol col, xrw height, xcol width)
			: xltype(xltypeSRef), val({.sref = {.count = 1, .ref = XREF<X>(row, col, height, width) }})
		{
		}

		// xltypeInt. Excel usually converts this to num.
		explicit XOPER(int w)
		{
			xltype = xltypeInt;
			val.w = static_cast<xint>(w);
		}
		bool operator==(int w) const
		{
			return (xltype & xltypeInt) ? val.w == w
				: (xltype & xltypeNum) ? val.num == w
				: (xltype & xltypeBool) ? val.xbool == w
				: false;
		}
		XOPER operator=(int w)
		{
			oper_free();
			operator=(XOPER(w));

			return *this;
		}

	private:
		// true if memory overlaps with x
		bool overlap(const X& x) const
		{
			return (begin() <= xll::begin(x) && xll::begin(x) < end()) 
				|| (begin() < xll::end(x)    && xll::end(x)  <= end());
		}

		void oper_free()
		{
			if (xltype & xlbitXLFree) {
				X* px[1];
				px[0] = this;
				ensure (xlretSuccess ==  traits<X>::Excelv(xlFree, 0, 1, px));
			}
			else if (xltype == xltypeStr) {
				str_free();
			}
			else if (xltype == xltypeMulti) {
				multi_free();
			}
			else if (xltype == xltypeRef) {
				ensure(!"oper_free: xltypeRef not implemented");
			}
			
			xltype = xltypeNil;
		}

		// allocate unless counted
		void str_alloc(xcstr str, size_t n)
		{
			xltype = xltypeStr;

			if (n == (size_t)-1) {
				val.str = const_cast<xchar*>(str); // move
			}
			else {
				val.str = (xchar*)malloc((n + 1) * sizeof(xchar));
				// first character is count
				if (val.str) {
					memcpy_s(val.str + 1, n * sizeof(xchar), str, n * sizeof(xchar));
					// ensure (n <= ...MAX);
					val.str[0] = static_cast<xchar>(n);
				}
			}
		}
		void str_append(xcstr str, size_t n)
		{
			if (xltype == xltypeNil) {
				str_alloc(str, n);
			}
			else if (xltype == (xltypeStr & xlbitXLFree)) {
				*this = XExcel<X>(xlfConcatenate, *this, XOPER<X>(str));
			}
			else {
				ensure(xltype == xltypeStr);
				bool counted = false;
				if (n == (size_t)-1) {
					counted = true;
					n = str[0];
				}
				if (n == 0) {
					n = traits<X>::len(str);
				}

				xchar* tmp = (xchar*)realloc(val.str, (val.str[0] + n + 1) * sizeof(xchar));
				if (!tmp) {
					throw std::bad_alloc{};
				}
				// ensure(tmp);
				val.str = tmp;
				memcpy_s(val.str + 1 + val.str[0], n * sizeof(xchar), str + counted, n * sizeof(xchar));
				val.str[0] = static_cast<xchar>(val.str[0] + n);

				if (counted) {
					free(const_cast<xchar*>(str));
				}
			}
		}
		void str_free()
		{
			ensure(xltype == xltypeStr);
			free(val.str);
			xltype = xltypeNil;
		}
		
		// xltypeMulti
		void multi_alloc(size_t r, size_t c)
		{
			ensure(r <= (size_t)std::numeric_limits<xrw>::max());
			ensure(c <= (size_t)std::numeric_limits<xcol>::max());

			if (r * c == 0) {
				xltype = XOPER().xltype;

				return;
			}

			xltype = xltypeMulti;
			val.array.rows = static_cast<xrw>(r);
			val.array.columns = static_cast<xcol>(c);
			val.array.lparray = static_cast<X*>(malloc(size() * sizeof(X)));
			if (val.array.lparray) {
				std::fill(begin(), end(), XOPER{});
			}
		}
		void multi_realloc(size_t r, size_t c)
		{
			ensure(r <= (size_t)std::numeric_limits<xrw>::max());
			ensure(c <= (size_t)std::numeric_limits<xcol>::max());

			ensure(xltype == xltypeMulti);

			if (r * c == 0) {
				multi_free();
				operator=(XOPER());

				return;
			}
	
			// current size
			auto n = size();
			val.array.rows = static_cast<xrw>(r);
			val.array.columns = static_cast<xcol>(c);
			if (n > size()) {
				std::for_each(end(), begin() + n, [](auto& o) { o.oper_free(); });
				X* tmp = (X*)realloc(val.array.lparray, size() * sizeof(X));
				ensure(tmp);
				val.array.lparray = tmp;
			}
			else if (n < size()) {
				X* tmp = (X*)realloc(val.array.lparray, size() * sizeof(X));
				ensure(tmp);
				val.array.lparray = tmp;
				std::fill(begin() + n, end(), XOPER{});
			}
		}
		void multi_free()
		{
			ensure(xltype == xltypeMulti);
			std::for_each(begin(), end(), [](auto& o) { o.oper_free(); });
			free(val.array.lparray);

			xltype = xltypeNil;
		}
	};

	typedef XOPER<XLOPER> OPER4;
	typedef XOPER<XLOPER12> OPER12;
	typedef XOPER<XLOPERX> OPER;

	typedef OPER4* LPOPER4;
	typedef OPER12* LPOPER12;
	typedef OPER* LPOPER;

}

// Just like Excel concatenation
inline xll::OPER4 operator&(const XLOPER& x, const XLOPER& y)
{
	return xll::OPER4(x) &= y;
}
inline xll::OPER4 operator&(const XLOPER& x, const char* y)
{
	xll::OPER4 z(x);

	return z.append(y);
}
inline xll::OPER12 operator&(const XLOPER12& x, const XLOPER12& y)
{
	return xll::OPER12(x) &= y;
}
inline xll::OPER12 operator&(const XLOPER12& x, const wchar_t* y)
{
	return xll::OPER12(x).append(y);
}

/*
// use xlfEvaluate???
inline auto& operator<<(std::ostream& os, const xll::OPER& o)
{
	switch (o.xltype) {
	case xltypeNum:
		os << o.val.num;
		break;
	case xltypeStr:
		os.write(o.val.str + 1, o.val.str[0]);
		break;
	case xltypeBool:
		os << static_cast<bool>(o.val.xbool);
		break;
	case xltypeErr:
		os << xll_err_str[o.val.err];
		break;
	case xltypeMulti: // "{a,b...;c,d,...}"
		os << "{";
		for (size_t r = 0; r < xll::rows(o); ++r) {
			for (size_t c = 0; c < xll::columns(o)) {
				if (c > 0)
					os << ",";
				else if (r > 0)
					os << ";";
				os << o(r, c);
			}
		}
		os << "}";
		break;
	case xltypeInt:
		os << o.val.w;
		break;
	default:
		break;
	}

	return os;
}
*/