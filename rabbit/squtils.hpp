/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

void *sq_vm_malloc(uint64_t size);
void *sq_vm_realloc(void *p,uint64_t oldsize,uint64_t size);
void sq_vm_free(void *p,uint64_t size);

#define sq_new(__ptr,__type) {__ptr=(__type *)sq_vm_malloc(sizeof(__type));new ((char*)__ptr) __type;}
#define sq_delete(__ptr,__type) {((__type*)__ptr)->~__type();sq_vm_free(__ptr,sizeof(__type));}

#define SQ_MALLOC(__size) sq_vm_malloc((__size));
#define SQ_FREE(__ptr,__size) sq_vm_free((__ptr),(__size));
#define SQ_REALLOC(__ptr,__oldsize,__size) sq_vm_realloc((__ptr),(__oldsize),(__size));

#define sq_aligning(v) (((size_t)(v) + (SQ_ALIGNMENT-1)) & (~(SQ_ALIGNMENT-1)))

//sqvector mini vector class, supports objects by value
template<typename T> class sqvector
{
public:
	sqvector()
	{
		_vals = NULL;
		_size = 0;
		_allocated = 0;
	}
	sqvector(const sqvector<T>& v)
	{
		copy(v);
	}
	void copy(const sqvector<T>& v)
	{
		if(_size) {
			resize(0); //destroys all previous stuff
		}
		//resize(v._size);
		if(v._size > _allocated) {
			_realloc(v._size);
		}
		for(uint64_t i = 0; i < v._size; i++) {
			new ((void *)&_vals[i]) T(v._vals[i]);
		}
		_size = v._size;
	}
	~sqvector()
	{
		if(_allocated) {
			for(uint64_t i = 0; i < _size; i++)
				_vals[i].~T();
			SQ_FREE(_vals, (_allocated * sizeof(T)));
		}
	}
	void reserve(uint64_t newsize) { _realloc(newsize); }
	void resize(uint64_t newsize, const T& fill = T())
	{
		if(newsize > _allocated)
			_realloc(newsize);
		if(newsize > _size) {
			while(_size < newsize) {
				new ((void *)&_vals[_size]) T(fill);
				_size++;
			}
		}
		else{
			for(uint64_t i = newsize; i < _size; i++) {
				_vals[i].~T();
			}
			_size = newsize;
		}
	}
	void shrinktofit() { if(_size > 4) { _realloc(_size); } }
	T& top() const { return _vals[_size - 1]; }
	inline uint64_t size() const { return _size; }
	bool empty() const { return (_size <= 0); }
	inline T &push_back(const T& val = T())
	{
		if(_allocated <= _size)
			_realloc(_size * 2);
		return *(new ((void *)&_vals[_size++]) T(val));
	}
	inline void pop_back()
	{
		_size--; _vals[_size].~T();
	}
	void insert(uint64_t idx, const T& val)
	{
		resize(_size + 1);
		for(uint64_t i = _size - 1; i > idx; i--) {
			_vals[i] = _vals[i - 1];
		}
		_vals[idx] = val;
	}
	void remove(uint64_t idx)
	{
		_vals[idx].~T();
		if(idx < (_size - 1)) {
			memmove(&_vals[idx], &_vals[idx+1], sizeof(T) * (_size - idx - 1));
		}
		_size--;
	}
	uint64_t capacity() { return _allocated; }
	inline T &back() const { return _vals[_size - 1]; }
	inline T& operator[](uint64_t pos) const{ return _vals[pos]; }
	T* _vals;
private:
	void _realloc(uint64_t newsize)
	{
		newsize = (newsize > 0)?newsize:4;
		_vals = (T*)SQ_REALLOC(_vals, _allocated * sizeof(T), newsize * sizeof(T));
		_allocated = newsize;
	}
	uint64_t _size;
	uint64_t _allocated;
};

