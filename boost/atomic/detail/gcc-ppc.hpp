#ifndef BOOST_DETAIL_ATOMIC_GCC_PPC_HPP
#define BOOST_DETAIL_ATOMIC_GCC_PPC_HPP

#include <boost/atomic/detail/base.hpp>
#include <boost/atomic/detail/builder.hpp>

/*
  Refer to: Motorola: "Programming Environments Manual for 32-Bit
  Implementations of the PowerPC Architecture", Appendix E:
  "Synchronization Programming Examples" for an explanation of what is
  going on here (can be found on the web at various places by the
  name "MPCFPE32B.pdf", Google is your friend...)
 */

namespace boost {
namespace detail {
namespace atomic {

static inline void __fence_before(memory_order order)
{
	switch(order) {
		case memory_order_release:
		case memory_order_acq_rel:
#if defined(__powerpc64__)
			__asm__ __volatile__ ("lwsync" ::: "memory");
#else
			__asm__ __volatile__ ("sync" ::: "memory");
#endif
			break;
		case memory_order_seq_cst:
			__asm__ __volatile__ ("sync" ::: "memory");
		default:;
	}
}

static inline void fence_after(memory_order order)
{
	switch(order) {
		case memory_order_acquire:
		case memory_order_acq_rel:
		case memory_order_seq_cst:
			__asm__ __volatile__ ("isync");
		case memory_order_consume:
			__asm__ __volatile__ ("" ::: "memory");
		default:;
	}
}

template<typename T>
static inline void fence_after_load(memory_order order, T value)
{
	switch(order) {
		case memory_order_acquire:
		case memory_order_acq_rel:
		case memory_order_seq_cst:
			/* perform a "fake" branch formally depending
			on the value loaded from memory; this will
			cause the subsequent "isync" to delay
			subsequent instructions until the load
			has finished */
			__asm__ __volatile__ (
				"cmpw %0, %0\n"
				"bne- 1f\n"
				"1f: isync\n"
				: "+b"(value));
		case memory_order_consume:
			__asm__ __volatile__ ("" ::: "memory");
		default:;
	}
}

/* note: the __asm__ constraint "b" instructs gcc to use any register
except r0; this is required because r0 is not allowed in
some places. Since I am sometimes unsure if it is allowed
or not just play it safe and avoid r0 entirely -- ppc isn't
exactly register-starved, so this really should not matter :) */

template<typename T>
class atomic_ppc_32 {
public:
	typedef T integral_type;
	explicit atomic_ppc_32(T v) : i(v) {}
	atomic_ppc_32() {}
	T load(memory_order order=memory_order_seq_cst) const volatile
	{
		T v=*reinterpret_cast<volatile const T *>(&i);
		fence_after_load(order, v);
		return v;
	}
	void store(T v, memory_order order=memory_order_seq_cst) volatile
	{
		__fence_before(order);
		*reinterpret_cast<volatile T *>(&i)=v;
	}
	bool compare_exchange_weak(
		T &expected,
		T desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		__fence_before(success_order);
		int success;
		__asm__ __volatile__(
			"lwarx %0,0,%2\n"
			"cmpw %0, %3\n"
			"bne- 2f\n"
			"stwcx. %4,0,%2\n"
			"bne- 2f\n"
			"addi %1,0,1\n"
			"1:"
			
			".subsection 2\n"
			"2: addi %1,0,0\n"
			"b 1b\n"
			".previous\n"
				: "=&b" (expected), "=&b" (success)
				: "b" (&i), "b" (expected), "b" ((int)desired)
			);
		if (success) fence_after(success_order);
		else fence_after(failure_order);
		return success;
	}
	
	bool is_lock_free(void) const volatile {return true;}
protected:
	inline T fetch_add_var(T c, memory_order order) volatile
	{
		__fence_before(order);
		T original, tmp;
		__asm__ __volatile__(
			"1: lwarx %0,0,%2\n"
			"add %1,%0,%3\n"
			"stwcx. %1,0,%2\n"
			"bne- 1b\n"
			: "=&b" (original), "=&b" (tmp)
			: "b" (&i), "b" (c)
			: "cc");
		fence_after(order);
		return original;
	}
	inline T fetch_inc(memory_order order) volatile
	{
		__fence_before(order);
		T original, tmp;
		__asm__ __volatile__(
			"1: lwarx %0,0,%2\n"
			"addi %1,%0,1\n"
			"stwcx. %1,0,%2\n"
			"bne- 1b\n"
			: "=&b" (original), "=&b" (tmp)
			: "b" (&i)
			: "cc");
		fence_after(order);
		return original;
	}
	inline T fetch_dec(memory_order order) volatile
	{
		__fence_before(order);
		T original, tmp;
		__asm__ __volatile__(
			"1: lwarx %0,0,%2\n"
			"addi %1,%0,-1\n"
			"stwcx. %1,0,%2\n"
			"bne- 1b\n"
			: "=&b" (original), "=&b" (tmp)
			: "b" (&i)
			: "cc");
		fence_after(order);
		return original;
	}
private:
	T i;
};

#if defined(__powerpc64__)

#warning Untested code -- please inform me if it works

template<typename T>
class atomic_ppc_64 {
public:
	typedef T integral_type;
	explicit atomic_ppc_64(T v) : i(v) {}
	atomic_ppc_64() {}
	T load(memory_order order=memory_order_seq_cst) const volatile
	{
		T v=*reinterpret_cast<volatile const T *>(&i);
		fence_after_load(order, v);
		return v;
	}
	void store(T v, memory_order order=memory_order_seq_cst) volatile
	{
		__fence_before(order);
		*reinterpret_cast<volatile T *>(&i)=v;
	}
	bool compare_exchange_weak(
		T &expected,
		T desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		__fence_before(success_order);
		int success;
		__asm__ __volatile__(
			"ldarx %0,0,%2\n"
			"cmpw %0, %3\n"
			"bne- 2f\n"
			"stdcx. %4,0,%2\n"
			"bne- 2f\n"
			"addi %1,0,1\n"
			"1:"
			
			".subsection 2\n"
			"2: addi %1,0,0\n"
			"b 1b\n"
			".previous\n"
				: "=&b" (expected), "=&b" (success)
				: "b" (&i), "b" (expected), "b" ((int)desired)
			);
		if (success) fence_after(success_order);
		else fence_after(failure_order);
		fence_after(order);
		return success;
	}
	
	bool is_lock_free(void) const volatile {return true;}
protected:
	inline T fetch_add_var(T c, memory_order order) volatile
	{
		__fence_before(order);
		T original, tmp;
		__asm__ __volatile__(
			"1: ldarx %0,0,%2\n"
			"add %1,%0,%3\n"
			"stdcx. %1,0,%2\n"
			"bne- 1b\n"
			: "=&b" (original), "=&b" (tmp)
			: "b" (&i), "b" (c)
			: "cc");
		fence_after(order);
		return original;
	}
	inline T fetch_inc(memory_order order) volatile
	{
		__fence_before(order);
		T original, tmp;
		__asm__ __volatile__(
			"1: ldarx %0,0,%2\n"
			"addi %1,%0,1\n"
			"stdcx. %1,0,%2\n"
			"bne- 1b\n"
			: "=&b" (original), "=&b" (tmp)
			: "b" (&i)
			: "cc");
		fence_after(order);
		return original;
	}
	inline T fetch_dec(memory_order order) volatile
	{
		__fence_before(order);
		T original, tmp;
		__asm__ __volatile__(
			"1: ldarx %0,0,%2\n"
			"addi %1,%0,-1\n"
			"stdcx. %1,0,%2\n"
			"bne- 1b\n"
			: "=&b" (original), "=&b" (tmp)
			: "b" (&i)
			: "cc");
		fence_after(order);
		return original;
	}
private:
	T i;
};
#endif

template<typename T>
class platform_atomic_integral<T, 4> : public build_atomic_from_typical<build_exchange<atomic_ppc_32<T> > > {
public:
	typedef build_atomic_from_typical<build_exchange<atomic_ppc_32<T> > > super;
	explicit platform_atomic_integral(T v) : super(v) {}
	platform_atomic_integral(void) {}
};

template<typename T>
class platform_atomic_integral<T, 1>: public build_atomic_from_larger_type<atomic_ppc_32<uint32_t>, T> {
public:
	typedef build_atomic_from_larger_type<atomic_ppc_32<uint32_t>, T> super;
	
	explicit platform_atomic_integral(T v) : super(v) {}
	platform_atomic_integral(void) {}
};

template<typename T>
class platform_atomic_integral<T, 2>: public build_atomic_from_larger_type<atomic_ppc_32<uint32_t>, T> {
public:
	typedef build_atomic_from_larger_type<atomic_ppc_32<uint32_t>, T> super;
	
	explicit platform_atomic_integral(T v) : super(v) {}
	platform_atomic_integral(void) {}
};

#if defined(__powerpc64__)
template<typename T>
class platform_atomic_integral<T, 8> : public build_atomic_from_typical<build_exchange<atomic_ppc_64<T> > > {
public:
	typedef build_atomic_from_typical<build_exchange<atomic_ppc_64<T> > > super;
	explicit platform_atomic_integral(T v) : super(v) {}
	platform_atomic_integral(void) {}
};
#endif

}
}
}

#endif
