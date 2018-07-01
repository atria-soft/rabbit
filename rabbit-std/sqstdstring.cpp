/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */

#include <rabbit/rabbit.hpp>
#include <rabbit-std/sqstdstring.hpp>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#define MAX_FORMAT_LEN  20
#define MAX_WFORMAT_LEN 3
#define ADDITIONAL_FORMAT_SPACE (100*sizeof(rabbit::Char))

static rabbit::Bool isfmtchr(rabbit::Char ch)
{
	switch(ch) {
		case '-':
		case '+':
		case ' ':
		case '#':
		case '0':
			return SQTrue;
	}
	return SQFalse;
}

static int64_t validate_format(rabbit::VirtualMachine* v, rabbit::Char *fmt, const rabbit::Char *src, int64_t n,int64_t &width)
{
	rabbit::Char *dummy;
	rabbit::Char swidth[MAX_WFORMAT_LEN];
	int64_t wc = 0;
	int64_t start = n;
	fmt[0] = '%';
	while (isfmtchr(src[n])) n++;
	while (scisdigit(src[n])) {
		swidth[wc] = src[n];
		n++;
		wc++;
		if(wc>=MAX_WFORMAT_LEN)
			return sq_throwerror(v,_SC("width format too long"));
	}
	swidth[wc] = '\0';
	if(wc > 0) {
		width = scstrtol(swidth,&dummy,10);
	}
	else
		width = 0;
	if (src[n] == '.') {
		n++;

		wc = 0;
		while (scisdigit(src[n])) {
			swidth[wc] = src[n];
			n++;
			wc++;
			if(wc>=MAX_WFORMAT_LEN)
				return sq_throwerror(v,_SC("precision format too long"));
		}
		swidth[wc] = '\0';
		if(wc > 0) {
			width += scstrtol(swidth,&dummy,10);

		}
	}
	if (n-start > MAX_FORMAT_LEN )
		return sq_throwerror(v,_SC("format too long"));
	memcpy(&fmt[1],&src[start],((n-start)+1)*sizeof(rabbit::Char));
	fmt[(n-start)+2] = '\0';
	return n;
}

rabbit::Result rabbit::std::format(rabbit::VirtualMachine* v,int64_t nformatstringidx,int64_t *outlen,rabbit::Char **output)
{
	const rabbit::Char *format;
	rabbit::Char *dest;
	rabbit::Char fmt[MAX_FORMAT_LEN];
	const rabbit::Result res = sq_getstring(v,nformatstringidx,&format);
	if (SQ_FAILED(res)) {
		return res; // propagate the error
	}
	int64_t format_size = sq_getsize(v,nformatstringidx);
	int64_t allocated = (format_size+2)*sizeof(rabbit::Char);
	dest = sq_getscratchpad(v,allocated);
	int64_t n = 0,i = 0, nparam = nformatstringidx+1, w = 0;
	//while(format[n] != '\0')
	while(n < format_size)
	{
		if(format[n] != '%') {
			assert(i < allocated);
			dest[i++] = format[n];
			n++;
		}
		else if(format[n+1] == '%') { //handles %%
				dest[i++] = '%';
				n += 2;
		}
		else {
			n++;
			if( nparam > sq_gettop(v) )
				return sq_throwerror(v,_SC("not enough parameters for the given format string"));
			n = validate_format(v,fmt,format,n,w);
			if(n < 0) return -1;
			int64_t addlen = 0;
			int64_t valtype = 0;
			const rabbit::Char *ts = NULL;
			int64_t ti = 0;
			float_t tf = 0;
			switch(format[n]) {
			case 's':
				if(SQ_FAILED(sq_getstring(v,nparam,&ts)))
					return sq_throwerror(v,_SC("string expected for the specified format"));
				addlen = (sq_getsize(v,nparam)*sizeof(rabbit::Char))+((w+1)*sizeof(rabbit::Char));
				valtype = 's';
				break;
			case 'i': case 'd': case 'o': case 'u':  case 'x':  case 'X':
#ifdef _SQ64
				{
				size_t flen = scstrlen(fmt);
				int64_t fpos = flen - 1;
				rabbit::Char f = fmt[fpos];
				const rabbit::Char *prec = (const rabbit::Char *)_PRINT_INT_PREC;
				while(*prec != _SC('\0')) {
					fmt[fpos++] = *prec++;
				}
				fmt[fpos++] = f;
				fmt[fpos++] = _SC('\0');
				}
#endif
			case 'c':
				if(SQ_FAILED(sq_getinteger(v,nparam,&ti)))
					return sq_throwerror(v,_SC("integer expected for the specified format"));
				addlen = (ADDITIONAL_FORMAT_SPACE)+((w+1)*sizeof(rabbit::Char));
				valtype = 'i';
				break;
			case 'f': case 'g': case 'G': case 'e':  case 'E':
				if(SQ_FAILED(sq_getfloat(v,nparam,&tf)))
					return sq_throwerror(v,_SC("float expected for the specified format"));
				addlen = (ADDITIONAL_FORMAT_SPACE)+((w+1)*sizeof(rabbit::Char));
				valtype = 'f';
				break;
			default:
				return sq_throwerror(v,_SC("invalid format"));
			}
			n++;
			allocated += addlen + sizeof(rabbit::Char);
			dest = sq_getscratchpad(v,allocated);
			switch(valtype) {
			case 's': i += scsprintf(&dest[i],allocated,fmt,ts); break;
			case 'i': i += scsprintf(&dest[i],allocated,fmt,ti); break;
			case 'f': i += scsprintf(&dest[i],allocated,fmt,tf); break;
			};
			nparam ++;
		}
	}
	*outlen = i;
	dest[i] = '\0';
	*output = dest;
	return SQ_OK;
}

static int64_t _string_printf(rabbit::VirtualMachine* v)
{
	rabbit::Char *dest = NULL;
	int64_t length = 0;
	if(SQ_FAILED(rabbit::std::format(v,2,&length,&dest)))
		return -1;

	SQPRINTFUNCTION printfunc = sq_getprintfunc(v);
	if(printfunc) printfunc(v,dest);

	return 0;
}

static int64_t _string_format(rabbit::VirtualMachine* v)
{
	rabbit::Char *dest = NULL;
	int64_t length = 0;
	if(SQ_FAILED(rabbit::std::format(v,2,&length,&dest)))
		return -1;
	sq_pushstring(v,dest,length);
	return 1;
}

static void __strip_l(const rabbit::Char *str,const rabbit::Char **start)
{
	const rabbit::Char *t = str;
	while(((*t) != '\0') && scisspace(*t)){ t++; }
	*start = t;
}

static void __strip_r(const rabbit::Char *str,int64_t len,const rabbit::Char **end)
{
	if(len == 0) {
		*end = str;
		return;
	}
	const rabbit::Char *t = &str[len-1];
	while(t >= str && scisspace(*t)) { t--; }
	*end = t + 1;
}

static int64_t _string_strip(rabbit::VirtualMachine* v)
{
	const rabbit::Char *str,*start,*end;
	sq_getstring(v,2,&str);
	int64_t len = sq_getsize(v,2);
	__strip_l(str,&start);
	__strip_r(str,len,&end);
	sq_pushstring(v,start,end - start);
	return 1;
}

static int64_t _string_lstrip(rabbit::VirtualMachine* v)
{
	const rabbit::Char *str,*start;
	sq_getstring(v,2,&str);
	__strip_l(str,&start);
	sq_pushstring(v,start,-1);
	return 1;
}

static int64_t _string_rstrip(rabbit::VirtualMachine* v)
{
	const rabbit::Char *str,*end;
	sq_getstring(v,2,&str);
	int64_t len = sq_getsize(v,2);
	__strip_r(str,len,&end);
	sq_pushstring(v,str,end - str);
	return 1;
}

static int64_t _string_split(rabbit::VirtualMachine* v)
{
	const rabbit::Char *str,*seps;
	rabbit::Char *stemp;
	sq_getstring(v,2,&str);
	sq_getstring(v,3,&seps);
	int64_t sepsize = sq_getsize(v,3);
	if(sepsize == 0) return sq_throwerror(v,_SC("empty separators string"));
	int64_t memsize = (sq_getsize(v,2)+1)*sizeof(rabbit::Char);
	stemp = sq_getscratchpad(v,memsize);
	memcpy(stemp,str,memsize);
	rabbit::Char *start = stemp;
	rabbit::Char *end = stemp;
	sq_newarray(v,0);
	while(*end != '\0')
	{
		rabbit::Char cur = *end;
		for(int64_t i = 0; i < sepsize; i++)
		{
			if(cur == seps[i])
			{
				*end = 0;
				sq_pushstring(v,start,-1);
				sq_arrayappend(v,-2);
				start = end + 1;
				break;
			}
		}
		end++;
	}
	if(end != start)
	{
		sq_pushstring(v,start,-1);
		sq_arrayappend(v,-2);
	}
	return 1;
}

static int64_t _string_escape(rabbit::VirtualMachine* v)
{
	const rabbit::Char *str;
	rabbit::Char *dest,*resstr;
	int64_t size;
	sq_getstring(v,2,&str);
	size = sq_getsize(v,2);
	if(size == 0) {
		sq_push(v,2);
		return 1;
	}
#ifdef SQUNICODE
#if WCHAR_SIZE == 2
	const rabbit::Char *escpat = _SC("\\x%04x");
	const int64_t maxescsize = 6;
#else //WCHAR_SIZE == 4
	const rabbit::Char *escpat = _SC("\\x%08x");
	const int64_t maxescsize = 10;
#endif
#else
	const rabbit::Char *escpat = _SC("\\x%02x");
	const int64_t maxescsize = 4;
#endif
	int64_t destcharsize = (size * maxescsize); //assumes every char could be escaped
	resstr = dest = (rabbit::Char *)sq_getscratchpad(v,destcharsize * sizeof(rabbit::Char));
	rabbit::Char c;
	rabbit::Char escch;
	int64_t escaped = 0;
	for(int n = 0; n < size; n++){
		c = *str++;
		escch = 0;
		if(scisprint(c) || c == 0) {
			switch(c) {
			case '\a': escch = 'a'; break;
			case '\b': escch = 'b'; break;
			case '\t': escch = 't'; break;
			case '\n': escch = 'n'; break;
			case '\v': escch = 'v'; break;
			case '\f': escch = 'f'; break;
			case '\r': escch = 'r'; break;
			case '\\': escch = '\\'; break;
			case '\"': escch = '\"'; break;
			case '\'': escch = '\''; break;
			case 0: escch = '0'; break;
			}
			if(escch) {
				*dest++ = '\\';
				*dest++ = escch;
				escaped++;
			}
			else {
				*dest++ = c;
			}
		}
		else {

			dest += scsprintf(dest, destcharsize, escpat, c);
			escaped++;
		}
	}

	if(escaped) {
		sq_pushstring(v,resstr,dest - resstr);
	}
	else {
		sq_push(v,2); //nothing escaped
	}
	return 1;
}

static int64_t _string_startswith(rabbit::VirtualMachine* v)
{
	const rabbit::Char *str,*cmp;
	sq_getstring(v,2,&str);
	sq_getstring(v,3,&cmp);
	int64_t len = sq_getsize(v,2);
	int64_t cmplen = sq_getsize(v,3);
	rabbit::Bool ret = SQFalse;
	if(cmplen <= len) {
		ret = memcmp(str,cmp,sq_rsl(cmplen)) == 0 ? SQTrue : SQFalse;
	}
	sq_pushbool(v,ret);
	return 1;
}

static int64_t _string_endswith(rabbit::VirtualMachine* v)
{
	const rabbit::Char *str,*cmp;
	sq_getstring(v,2,&str);
	sq_getstring(v,3,&cmp);
	int64_t len = sq_getsize(v,2);
	int64_t cmplen = sq_getsize(v,3);
	rabbit::Bool ret = SQFalse;
	if(cmplen <= len) {
		ret = memcmp(&str[len - cmplen],cmp,sq_rsl(cmplen)) == 0 ? SQTrue : SQFalse;
	}
	sq_pushbool(v,ret);
	return 1;
}

#define SETUP_REX(v) \
	rabbit::std::SQRex *self = NULL; \
	rabbit::sq_getinstanceup(v,1,(rabbit::UserPointer *)&self,0);

static int64_t _rexobj_releasehook(rabbit::UserPointer p, int64_t SQ_UNUSED_ARG(size))
{
	rabbit::std::SQRex *self = ((rabbit::std::SQRex *)p);
	rabbit::std::rex_free(self);
	return 1;
}

static int64_t _regexp_match(rabbit::VirtualMachine* v)
{
	SETUP_REX(v);
	const rabbit::Char *str;
	sq_getstring(v,2,&str);
	if(rabbit::std::rex_match(self,str) == SQTrue)
	{
		sq_pushbool(v,SQTrue);
		return 1;
	}
	sq_pushbool(v,SQFalse);
	return 1;
}

static void _addrexmatch(rabbit::VirtualMachine* v,const rabbit::Char *str,const rabbit::Char *begin,const rabbit::Char *end)
{
	sq_newtable(v);
	sq_pushstring(v,_SC("begin"),-1);
	sq_pushinteger(v,begin - str);
	sq_rawset(v,-3);
	sq_pushstring(v,_SC("end"),-1);
	sq_pushinteger(v,end - str);
	sq_rawset(v,-3);
}

static int64_t _regexp_search(rabbit::VirtualMachine* v)
{
	SETUP_REX(v);
	const rabbit::Char *str,*begin,*end;
	int64_t start = 0;
	sq_getstring(v,2,&str);
	if(sq_gettop(v) > 2) sq_getinteger(v,3,&start);
	if(rabbit::std::rex_search(self,str+start,&begin,&end) == SQTrue) {
		_addrexmatch(v,str,begin,end);
		return 1;
	}
	return 0;
}

static int64_t _regexp_capture(rabbit::VirtualMachine* v)
{
	SETUP_REX(v);
	const rabbit::Char *str,*begin,*end;
	int64_t start = 0;
	sq_getstring(v,2,&str);
	if(sq_gettop(v) > 2) sq_getinteger(v,3,&start);
	if(rabbit::std::rex_search(self,str+start,&begin,&end) == SQTrue) {
		int64_t n = rabbit::std::rex_getsubexpcount(self);
		rabbit::std::SQRexMatch match;
		sq_newarray(v,0);
		for(int64_t i = 0;i < n; i++) {
			rabbit::std::rex_getsubexp(self,i,&match);
			if(match.len > 0)
				_addrexmatch(v,str,match.begin,match.begin+match.len);
			else
				_addrexmatch(v,str,str,str); //empty match
			sq_arrayappend(v,-2);
		}
		return 1;
	}
	return 0;
}

static int64_t _regexp_subexpcount(rabbit::VirtualMachine* v)
{
	SETUP_REX(v);
	sq_pushinteger(v,rabbit::std::rex_getsubexpcount(self));
	return 1;
}

static int64_t _regexp_constructor(rabbit::VirtualMachine* v)
{
	const rabbit::Char *error,*pattern;
	sq_getstring(v,2,&pattern);
	rabbit::std::SQRex *rex = rabbit::std::rex_compile(pattern,&error);
	if(!rex) return sq_throwerror(v,error);
	sq_setinstanceup(v,1,rex);
	sq_setreleasehook(v,1,_rexobj_releasehook);
	return 0;
}

static int64_t _regexp__typeof(rabbit::VirtualMachine* v)
{
	sq_pushstring(v,_SC("regexp"),-1);
	return 1;
}

#define _DECL_REX_FUNC(name,nparams,pmask) {_SC(#name),_regexp_##name,nparams,pmask}
static const rabbit::RegFunction rexobj_funcs[]={
	_DECL_REX_FUNC(constructor,2,_SC(".s")),
	_DECL_REX_FUNC(search,-2,_SC("xsn")),
	_DECL_REX_FUNC(match,2,_SC("xs")),
	_DECL_REX_FUNC(capture,-2,_SC("xsn")),
	_DECL_REX_FUNC(subexpcount,1,_SC("x")),
	_DECL_REX_FUNC(_typeof,1,_SC("x")),
	{NULL,(SQFUNCTION)0,0,NULL}
};
#undef _DECL_REX_FUNC

#define _DECL_FUNC(name,nparams,pmask) {_SC(#name),_string_##name,nparams,pmask}
static const rabbit::RegFunction stringlib_funcs[]={
	_DECL_FUNC(format,-2,_SC(".s")),
	_DECL_FUNC(printf,-2,_SC(".s")),
	_DECL_FUNC(strip,2,_SC(".s")),
	_DECL_FUNC(lstrip,2,_SC(".s")),
	_DECL_FUNC(rstrip,2,_SC(".s")),
	_DECL_FUNC(split,3,_SC(".ss")),
	_DECL_FUNC(escape,2,_SC(".s")),
	_DECL_FUNC(startswith,3,_SC(".ss")),
	_DECL_FUNC(endswith,3,_SC(".ss")),
	{NULL,(SQFUNCTION)0,0,NULL}
};
#undef _DECL_FUNC


int64_t rabbit::std::register_stringlib(rabbit::VirtualMachine* v)
{
	sq_pushstring(v,_SC("regexp"),-1);
	sq_newclass(v,SQFalse);
	int64_t i = 0;
	while(rexobj_funcs[i].name != 0) {
		const rabbit::RegFunction &f = rexobj_funcs[i];
		sq_pushstring(v,f.name,-1);
		sq_newclosure(v,f.f,0);
		sq_setparamscheck(v,f.nparamscheck,f.typemask);
		sq_setnativeclosurename(v,-1,f.name);
		sq_newslot(v,-3,SQFalse);
		i++;
	}
	sq_newslot(v,-3,SQFalse);

	i = 0;
	while(stringlib_funcs[i].name!=0)
	{
		sq_pushstring(v,stringlib_funcs[i].name,-1);
		sq_newclosure(v,stringlib_funcs[i].f,0);
		sq_setparamscheck(v,stringlib_funcs[i].nparamscheck,stringlib_funcs[i].typemask);
		sq_setnativeclosurename(v,-1,stringlib_funcs[i].name);
		sq_newslot(v,-3,SQFalse);
		i++;
	}
	return 1;
}
