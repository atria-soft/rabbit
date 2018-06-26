/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

struct SQUserData : SQDelegable
{
    SQUserData(SQSharedState *ss){ _delegate = 0; _hook = NULL; INIT_CHAIN(); ADD_TO_CHAIN(&_ss(this)->_gc_chain, this); }
    ~SQUserData()
    {
        REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain, this);
        SetDelegate(NULL);
    }
    static SQUserData* Create(SQSharedState *ss, SQInteger size)
    {
        SQUserData* ud = (SQUserData*)SQ_MALLOC(sq_aligning(sizeof(SQUserData))+size);
        new (ud) SQUserData(ss);
        ud->_size = size;
        ud->_typetag = 0;
        return ud;
    }
    void Release() {
        if (_hook) _hook((SQUserPointer)sq_aligning(this + 1),_size);
        SQInteger tsize = _size;
        this->~SQUserData();
        SQ_FREE(this, sq_aligning(sizeof(SQUserData)) + tsize);
    }


    SQInteger _size;
    SQRELEASEHOOK _hook;
    SQUserPointer _typetag;
    //SQChar _val[1];
};

