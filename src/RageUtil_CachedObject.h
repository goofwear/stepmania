/* CachedObject - cached object pointers with automatic invalidation. */

#ifndef RAGE_UTIL_CACHED_OBJECT_H
#define RAGE_UTIL_CACHED_OBJECT_H

#include "Foreach.h"

template<typename T>
class CachedObjectPointer;

namespace CachedObjectHelpers
{
	void Lock();
	void Unlock();
}

template<typename T>
class CachedObject
{
public:
	CachedObject()
	{
		m_pObject = NULL;

		/* A new object is being constructed, so invalidate negative caching. */
		ClearCacheNegative();
	}

	CachedObject( const CachedObject &cpy )
	{
		m_pObject = NULL;
		ClearCacheNegative();
	}

	~CachedObject()
	{
		if( m_pObject != NULL )
			ClearCacheSpecific( m_pObject );
	}

	CachedObject &operator=( const CachedObject &rhs ) { return *this; }

	/* Clear all cached entries for this type. */
	static void ClearCacheAll()
	{
		CachedObjectHelpers::Lock();
		for( typename vector<ObjectPointer *>::iterator p = m_apObjectPointers.begin(); p != m_apObjectPointers.end(); ++p )
		{
			(*p)->m_pCache = NULL;
			(*p)->m_bCacheIsSet = false;
		}
		CachedObjectHelpers::Unlock();
	}

	/* Clear all cached entries pointing to a specific object. */
	static void ClearCacheSpecific( const T *pObject )
	{
		CachedObjectHelpers::Lock();
		for( typename vector<ObjectPointer *>::iterator p = m_apObjectPointers.begin(); p != m_apObjectPointers.end(); ++p )
		{
			if( (*p)->m_pCache == pObject )
			{
				(*p)->m_pCache = NULL;
				(*p)->m_bCacheIsSet = false;
			}
		}
		CachedObjectHelpers::Unlock();
	}

	/* Clear all negative cached entries of this type. */
	static void ClearCacheNegative()
	{
		CachedObjectHelpers::Lock();
		for( typename vector<ObjectPointer *>::iterator p = m_apObjectPointers.begin(); p != m_apObjectPointers.end(); ++p )
		{
			if( (*p)->m_pCache == NULL )
				(*p)->m_bCacheIsSet = false;
		}
		CachedObjectHelpers::Unlock();
	}

private:
	typedef CachedObjectPointer<T> ObjectPointer;
	friend class CachedObjectPointer<T>;

	static void Register( ObjectPointer *p )
	{
		m_apObjectPointers.push_back( p );
	}

	static void Unregister( ObjectPointer *p )
	{
		typename vector<ObjectPointer *>::iterator it = find( m_apObjectPointers.begin(), m_apObjectPointers.end(), p );
		ASSERT( it != m_apObjectPointers.end() );
		m_apObjectPointers.erase( it );
	}

	/* This points to the actual T this object is contained in.  This is set
	 * the first time CachedObjectPointer::Set() is called for this object.
	 * That's more convenient than setting it ourselves; we don't need to
	 * do anything special in T's copy ctor.  This works because there's no
	 * need to clear cache for an object before any CachedObjectPointers have
	 * ever been set for it. */
	const T *m_pObject;
	static vector<ObjectPointer *> m_apObjectPointers;
};
template<typename T> vector<CachedObjectPointer<T> *> CachedObject<T>::m_apObjectPointers = vector<CachedObjectPointer<T> *>();

template<typename T>
class CachedObjectPointer
{
public:
	typedef CachedObject<T> Object;

	CachedObjectPointer()
	{
		m_bCacheIsSet = false;
		Object::Register( this );
	}

	CachedObjectPointer( const CachedObjectPointer &cpy )
	{
		CachedObjectHelpers::Lock();
		m_pCache = cpy.m_pCache;
		m_bCacheIsSet = cpy.m_bCacheIsSet;
		Object::Register( this );
		CachedObjectHelpers::Unlock();
	}

	~CachedObjectPointer()
	{
		Object::Unregister( this );
	}

	bool Get( T **pRet ) const
	{
		CachedObjectHelpers::Lock();
		if( !m_bCacheIsSet )
		{
			CachedObjectHelpers::Unlock();
			return false;
		}
		*pRet = m_pCache;
		CachedObjectHelpers::Unlock();
		return true;
	}

	void Set( T *p )
	{
		CachedObjectHelpers::Lock();
		m_pCache = p;
		m_bCacheIsSet = true;
		if( p != NULL )
			p->m_CachedObject.m_pObject = p;
		CachedObjectHelpers::Unlock();
	}

	void Unset()
	{
		CachedObjectHelpers::Lock();
		m_pCache = NULL;
		m_bCacheIsSet = false;
		CachedObjectHelpers::Unlock();
	}

private:
	friend class CachedObject<T>;

	T *m_pCache;
	bool m_bCacheIsSet;
};

#define CACHED_REGISTER_CLASS(T)

#endif

/*
 * (c) 2007 Glenn Maynard
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
