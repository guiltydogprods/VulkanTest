#pragma once

struct Finalizer
{
	void(*fn)(void *ptr);
	Finalizer *chain;
};

template <typename T>
void destructorCall(void *ptr)
{
	static_cast<T*>(ptr)->~T();
}

class ScopeStack
{
public:
	explicit ScopeStack(LinearAllocator& a, const char *scopeName = nullptr)
		: m_alloc(a)
		, m_rewindPoint(a.ptr())
		, m_finalizerChain(nullptr)
	{
#ifdef MEM_DEBUG
		if (scopeName)
		{
			strcpy_s(m_scopeName, sizeof(m_scopeName), scopeName);
			print("Push Scope: (0x%016x) %s\n", (size_t)this, m_scopeName);
			if (strlen(scopeName) < 1)
			{
				scopeName = (scopeName + 1) - 1;
			}
		}
		else
		{
			print("Push Scope: (0x%016x) unnamed\n", (size_t)this);
		}
#endif
	}

	explicit ScopeStack(ScopeStack& oldScope, const char *scopeName = nullptr)
		: m_alloc(oldScope.m_alloc)
		, m_rewindPoint(oldScope.m_alloc.ptr())
		, m_finalizerChain(nullptr)
	{
#ifdef MEM_DEBUG
		if (scopeName)
		{
			strcpy_s(m_scopeName, sizeof(m_scopeName), scopeName);
			print("Push Scope: (0x%016x) %s\n", (size_t)this, m_scopeName);
			if (strlen(scopeName) < 1)
			{
				scopeName = (scopeName + 1) - 1;
			}
		}
		else
		{
			print("Push Scope: (0x%016x) unnamed\n", (size_t)this);
		}
#endif
	}

	~ScopeStack()
	{
#ifdef MEM_DEBUG
		print("Pop Scope: (0x%016x) %s\n", (size_t)this, m_scopeName);
#endif
		for (Finalizer *f = m_finalizerChain; f; f = f->chain)
		{
			(*f->fn)(objectFromFinalizer(f));
		}
		m_alloc.rewind(m_rewindPoint);
	}
#if 1
	template <typename T, typename... Args>
	T* newObject(Args&&... params)
	{
		// Allocate memory for finalizer + object.
		Finalizer* f = allocWithFinalizer(sizeof(T));

		// Placement construct object in space after finalizer. Do this before
		// linking in the finalizer for this object so nested calls will be
		// finalized after this object.
		//		void *allocAddress = objectFromFinalizer(f);
		T* result = new (objectFromFinalizer(f)) T(std::forward<Args>(params)...);

		// Link this finalizer onto the chain.
		f->fn = &destructorCall<T>;
		f->chain = m_finalizerChain;
		m_finalizerChain = f;
		return result;
	}
#endif
#if 0
	template <typename T>
	T* newObject() 
	{
		// Allocate memory for finalizer + object.
		Finalizer* f = allocWithFinalizer(sizeof(T));

		// Placement construct object in space after finalizer. Do this before
		// linking in the finalizer for this object so nested calls will be
		// finalized after this object.
		T* result = new (objectFromFinalizer(f)) T;

		// Link this finalizer onto the chain.
		f->fn = &destructorCall<T>;
		f->chain = m_finalizerChain;
		m_finalizerChain = f;
		return result;
	}
#endif
	template <typename T>
	T* newPOD()
	{
		return new (m_alloc.allocate(sizeof(T))) T;
	}

	void *allocate(size_t size, size_t align = 16)
	{
		return m_alloc.allocate(size, align);
	}

private:
	LinearAllocator& m_alloc;
	void *m_rewindPoint;
	Finalizer *m_finalizerChain;
#ifdef MEM_DEBUG
	char m_scopeName[32];
#endif
	static void* objectFromFinalizer(Finalizer *f)
	{
		return ((uint8_t*)f) + LinearAllocator::alignedSize(sizeof(Finalizer));
	}

	Finalizer *allocWithFinalizer(size_t size)
	{
		return (Finalizer*)m_alloc.allocate(size + LinearAllocator::alignedSize(sizeof(Finalizer)));
	}
};
