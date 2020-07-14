// handle.cpp - Illustrate the use of handles
//#define XLOPERX XLOPER
#include "../xll/xll.h"

using namespace xll;

class base {
	OPERX x;
public:
	base() { }
	base(const OPERX& x) : x(x) { }
	base(const base&) = default;
	base(base&&) = default;
	base& operator=(const base&) = default;
	base& operator=(base&&) = default;
	virtual ~base() { }
	OPERX& get() { return x; }
};

AddInX xai_base(
	FunctionX(XLL_DOUBLEX, X_("?xll_base"), X_("XLL.BASE"))
	.Args({
		ArgX({ XLL_LPOPERX, X_("x"), X_("is a cell or range of cells") })
	})
	.FunctionHelp(X_("Return a handle to a base object."))
	.Uncalced() // required for functions creating handles
);
HANDLEX WINAPI xll_base(LPOPERX px)
{
#pragma XLLEXPORT
	xll::handle h(new base(*px));

	return h.get();
}

AddInX xai_base_get(
	FunctionX(XLL_LPOPERX, X_("?xll_base_get"), X_("XLL.BASE.GET"))
	.Args({
		ArgX({ XLL_DOUBLEX, X_("handle"), X_("is a handle returned by XLL.BASE") })
	})
	.FunctionHelp(X_("Return the value stored in base."))
);
LPOPERX WINAPI xll_base_get(HANDLEX _h)
{
#pragma XLLEXPORT
	xll::handle<base> h(_h);

	return &(h->get());
	//return h ? &h->get() : &ErrNAX;
}
