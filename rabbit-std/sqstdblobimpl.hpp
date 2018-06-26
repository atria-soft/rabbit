/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

struct SQBlob : public SQStream
{
	SQBlob(int64_t size) {
		_size = size;
		_allocated = size;
		_buf = (unsigned char *)sq_malloc(size);
		memset(_buf, 0, _size);
		_ptr = 0;
		_owns = true;
	}
	virtual ~SQBlob() {
		sq_free(_buf, _allocated);
	}
	int64_t Write(void *buffer, int64_t size) {
		if(!CanAdvance(size)) {
			GrowBufOf(_ptr + size - _size);
		}
		memcpy(&_buf[_ptr], buffer, size);
		_ptr += size;
		return size;
	}
	int64_t Read(void *buffer,int64_t size) {
		int64_t n = size;
		if(!CanAdvance(size)) {
			if((_size - _ptr) > 0)
				n = _size - _ptr;
			else return 0;
		}
		memcpy(buffer, &_buf[_ptr], n);
		_ptr += n;
		return n;
	}
	bool resize(int64_t n) {
		if(!_owns) return false;
		if(n != _allocated) {
			unsigned char *newbuf = (unsigned char *)sq_malloc(n);
			memset(newbuf,0,n);
			if(_size > n)
				memcpy(newbuf,_buf,n);
			else
				memcpy(newbuf,_buf,_size);
			sq_free(_buf,_allocated);
			_buf=newbuf;
			_allocated = n;
			if(_size > _allocated)
				_size = _allocated;
			if(_ptr > _allocated)
				_ptr = _allocated;
		}
		return true;
	}
	bool GrowBufOf(int64_t n)
	{
		bool ret = true;
		if(_size + n > _allocated) {
			if(_size + n > _size * 2)
				ret = resize(_size + n);
			else
				ret = resize(_size * 2);
		}
		_size = _size + n;
		return ret;
	}
	bool CanAdvance(int64_t n) {
		if(_ptr+n>_size)return false;
		return true;
	}
	int64_t Seek(int64_t offset, int64_t origin) {
		switch(origin) {
			case SQ_SEEK_SET:
				if(offset > _size || offset < 0) return -1;
				_ptr = offset;
				break;
			case SQ_SEEK_CUR:
				if(_ptr + offset > _size || _ptr + offset < 0) return -1;
				_ptr += offset;
				break;
			case SQ_SEEK_END:
				if(_size + offset > _size || _size + offset < 0) return -1;
				_ptr = _size + offset;
				break;
			default: return -1;
		}
		return 0;
	}
	bool IsValid() {
		return _size == 0 || _buf?true:false;
	}
	bool EOS() {
		return _ptr == _size;
	}
	int64_t Flush() { return 0; }
	int64_t Tell() { return _ptr; }
	int64_t Len() { return _size; }
	SQUserPointer getBuf(){ return _buf; }
private:
	int64_t _size;
	int64_t _allocated;
	int64_t _ptr;
	unsigned char *_buf;
	bool _owns;
};

