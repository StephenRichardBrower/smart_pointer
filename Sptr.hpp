/*
 * Sptr.hpp
 *
 *  Created on: Apr 5, 2013
 *      Author: Stephen Brower
 */

#ifndef SPTR_HPP_
#define SPTR_HPP_

#include <atomic>
#include <stdint.h>
#include <iostream>
#include <pthread.h>
#include <assert.h>
#include <mutex>

namespace cs540{
	std::mutex thread_lock;

	struct ReferenceObject{
	public:
		size_t refcount;
		void* original_pointer;
		int inc(){int inc; thread_lock.lock();inc = ++refcount;thread_lock.unlock();return inc;}
		int dec(){int dec; thread_lock.lock();dec = --refcount;thread_lock.unlock();return dec;}
		virtual ~ReferenceObject(){}
	};

	template <typename T>
	struct ReferenceEraser: public ReferenceObject{
		T* original_pointer;
		ReferenceEraser(T* ptr){/*pthread_mutex_init(&lock,NULL);*/ thread_lock.lock();refcount = 1; original_pointer = ptr;thread_lock.unlock();}
		~ReferenceEraser(){thread_lock.lock();delete original_pointer; thread_lock.unlock(); /*pthread_mutex_destroy(&lock)*/;}
	};

	template <typename T>
	class Sptr{

	public:
		ReferenceObject* refcounter;
		T* current_pointer;

	public:
		//Constructs a smart pointer that points to the null pointer.
		Sptr():
			refcounter(nullptr), 
			current_pointer(nullptr)
			{}

		//Constructs a smart pointer that points to the given object. The reference count is initialized to one.
		template <typename U>
		Sptr(U* arg_ptr)
		{
			refcounter = new ReferenceEraser<U>(arg_ptr);
			refcounter->original_pointer = arg_ptr;
			current_pointer = arg_ptr;
		}

		//The reference count is incremented. If U * is not implicitly convertible to T *, this will result in a syntax error. Note that both the normal copy constructor and a member template copy constructor must be provided for proper operation.
		Sptr(const Sptr &o){
			refcounter = o.refcounter;
			if (o.refcounter){
				refcounter->inc();
				current_pointer = o.current_pointer;
			}
			else{
				refcounter = nullptr;
				current_pointer = nullptr;
			}
		}

		template <typename U> Sptr(const Sptr<U> &o){
			refcounter = o.refcounter;
			if (o.refcounter){
				refcounter->inc();
				current_pointer = static_cast<T*>(o.current_pointer);
			}
			else{
				refcounter = nullptr;
				current_pointer = nullptr;
			}
		}

		template <typename U> Sptr(const Sptr<U> &o, bool thisisdumb){
			if (o.refcounter){
				refcounter = o.refcounter;
				refcounter->inc();
				current_pointer = dynamic_cast<T*>(o.current_pointer);
			}
			else{
				refcounter = nullptr;
				current_pointer = nullptr;
			}
		}

/*

Okay, I see at least one problem.  You need to make sure that any access to any data that can occur from multiple threads concurrently is protected.  It looks to me like this read of refcount is not protected.

Sptr &operator=(const Sptr &o){
    if (&o == this){return *this;}
    if (refcounter != nullptr){
            if (refcounter->refcount == 1){*/
		
		Sptr &operator=(const Sptr &o){
			if (&o == this){return *this;}
			if (refcounter != nullptr){
			    thread_lock.lock();		    
				if (refcounter->refcount == 1){
				    thread_lock.unlock();
					delete refcounter;
					refcounter = nullptr;
					current_pointer = nullptr;
				}
				else {
				    thread_lock.unlock();
					refcounter->dec();
				}
			}
			if (o.refcounter != nullptr){
				refcounter =  o.refcounter;
				refcounter->inc();
				current_pointer = o.current_pointer;
			}
			else {
				refcounter = nullptr;
				current_pointer = nullptr;
			}
			return *this;
		}

		template <typename U>
		Sptr<T> &operator=(const Sptr<U> &o){
			if (&o == (Sptr<U>*) this){return *this;}
			if (refcounter != nullptr){
			    thread_lock.lock();
				if (refcounter->refcount == 1){
				    thread_lock.unlock();
					delete refcounter;
					refcounter = nullptr;
					current_pointer = nullptr;
				}
				else {
				    thread_lock.unlock();
					refcounter->dec();
				}
			}
			if (o.refcounter != nullptr){
				refcounter =  o.refcounter;
				refcounter->inc();
				current_pointer = static_cast<T*>(o.current_pointer);
			}
			else {
				refcounter = nullptr;
				current_pointer = nullptr;
			}
			return *this;
		}

		//The smart pointer is set to point to the null pointer. The reference count for the currently pointed to object, if any, is decremented.
		void reset(){
			if (refcounter != nullptr){
				if ((refcounter)->refcount == 1){
					delete refcounter;
				}
				else{
					refcounter->dec();
				}
				refcounter = nullptr;
			}
			current_pointer = nullptr;
		}

		//A reference to the pointed-to object is returned. Note that this will be a const-reference if T is a const-qualified type.
		T &operator*() const{
			return *current_pointer;
		}
		//The pointer is returned. Note that this will be a pointer-to-const if T is a const-qualified type.
		T *operator->() const{
			return current_pointer;
		}
		//The pointer is returned. Note that this will be a pointer-to-const if T is a const-qualified type.
		T *get() const{
			return (current_pointer);
		}

		explicit operator bool() const{
			return (current_pointer != nullptr);
		}

		~Sptr(){
			reset();
		}
	};
	
	//Returns true if the two smart pointers point to the same object or if they are both null. Note that implicit conversions may need to be applied.
	template <typename T1, typename T2>
	bool operator==(const Sptr<T1> &ptr1, const Sptr<T2> &ptr2){
		return (ptr1.get() == ptr2.get());
	}

	//Convert sp by using static_cast to cast the contained pointer. It will result in a syntax error if static_cast cannot be applied to the relevant types.
	template <typename T, typename U>
	Sptr<T> static_pointer_cast(const Sptr<U> &sp){
		Sptr<T> temp(sp);
		{temp.current_pointer = static_cast<T*>(sp.current_pointer);}
		return temp;
	}

	template <typename T, typename U>
	Sptr<T> dynamic_pointer_cast(const Sptr<U> &sp){
		Sptr<T> temp(sp, true);
		temp.current_pointer = dynamic_cast<T*>(sp.current_pointer);
		//else {temp.current_pointer = dynamic_cast<T*>(((ReferenceEraser<U>*)sp.refcounter)->original_pointer);}
		return temp;
	}

}

#endif /* SPTR_HPP_ */
