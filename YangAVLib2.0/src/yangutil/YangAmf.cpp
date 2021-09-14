#include "yangutil/sys/YangAmf.h"

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "yangutil/sys/YangLog.h"
enum Yang_LogLevel {
	Yang_LOGCRIT = 0,
	Yang_LOGERROR,
	Yang_LOGWARNING,
	Yang_LOGINFO,
	Yang_LOGDEBUG,
	Yang_LOGDEBUG2,
	Yang_LOGALL
};
void Yang_Logs1(int32_t level, const char *format, ...) {
	if (level > Yang_LOGERROR)
		return;
	char str[2048] = "";
	va_list args;
	va_start(args, format);
	vsnprintf(str, 2048 - 1, format, args);
	va_end(args);
}
#ifdef _WIN32
/* Windows is little endian only */
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __BYTE_ORDER __LITTLE_ENDIAN
#define __FLOAT_WORD_ORDER __BYTE_ORDER

typedef uint8_t uint8_t;

#else /* !_WIN32 */

#include <sys/param.h>

#if defined(BYTE_ORDER) && !defined(__BYTE_ORDER)
#define __BYTE_ORDER    BYTE_ORDER
#endif

#if defined(BIG_ENDIAN) && !defined(__BIG_ENDIAN)
#define __BIG_ENDIAN	BIG_ENDIAN
#endif

#if defined(LITTLE_ENDIAN) && !defined(__LITTLE_ENDIAN)
#define __LITTLE_ENDIAN	LITTLE_ENDIAN
#endif

#endif /* !_WIN32 */

/* define default endianness */
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN	1234
#endif

#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN	4321
#endif

#ifndef __BYTE_ORDER
#warning "Byte order not defined on your system, assuming little endian!"
#define __BYTE_ORDER	__LITTLE_ENDIAN
#endif

/* ok, we assume to have the same float word order and byte order if float word order is not defined */
#ifndef __FLOAT_WORD_ORDER
#warning "Float word order not defined, assuming the same as byte order!"
#define __FLOAT_WORD_ORDER	__BYTE_ORDER
#endif

#if !defined(__BYTE_ORDER) || !defined(__FLOAT_WORD_ORDER)
#error "Undefined byte or float word order!"
#endif

#if __FLOAT_WORD_ORDER != __BIG_ENDIAN && __FLOAT_WORD_ORDER != __LITTLE_ENDIAN
#error "Unknown/unsupported float word order!"
#endif

#if __BYTE_ORDER != __BIG_ENDIAN && __BYTE_ORDER != __LITTLE_ENDIAN
#error "Unknown/unsupported byte order!"
#endif

static const AMFObjectProperty AMFProp_Invalid = { { 0, 0 }, AMF_INVALID };
static const AMFObject AMFObj_Invalid = { 0, 0 };
static const AVal AV_empty = { 0, 0 };

/* Data is Big-Endian */
unsigned short AMF_DecodeInt16(const char *data) {
	uint8_t *c = (uint8_t*) data;
	unsigned short val;
	val = (c[0] << 8) | c[1];
	return val;
}

uint32_t  AMF_DecodeInt24(const char *data) {
	uint8_t *c = (uint8_t*) data;
	uint32_t  val;
	val = (c[0] << 16) | (c[1] << 8) | c[2];
	return val;
}

uint32_t  AMF_DecodeInt32(const char *data) {
	uint8_t *c = (uint8_t*) data;
	uint32_t  val;
	val = (c[0] << 24) | (c[1] << 16) | (c[2] << 8) | c[3];
	return val;
}

void AMF_DecodeString(const char *data, AVal *bv) {
	bv->av_len = AMF_DecodeInt16(data);
	bv->av_val = (bv->av_len > 0) ? (char*) data + 2 : NULL;
}

void AMF_DecodeLongString(const char *data, AVal *bv) {
	bv->av_len = AMF_DecodeInt32(data);
	bv->av_val = (bv->av_len > 0) ? (char*) data + 4 : NULL;
}

double AMF_DecodeNumber(const char *data) {
	double dVal;
#if __FLOAT_WORD_ORDER == __BYTE_ORDER
#if __BYTE_ORDER == __BIG_ENDIAN
  memcpy(&dVal, data, 8);
#elif __BYTE_ORDER == __LITTLE_ENDIAN
	uint8_t *ci, *co;
	ci = (uint8_t*) data;
	co = (uint8_t*) &dVal;
	co[0] = ci[7];
	co[1] = ci[6];
	co[2] = ci[5];
	co[3] = ci[4];
	co[4] = ci[3];
	co[5] = ci[2];
	co[6] = ci[1];
	co[7] = ci[0];
#endif
#else
#if __BYTE_ORDER == __LITTLE_ENDIAN	/* __FLOAT_WORD_ORER == __BIG_ENDIAN */
  uint8_t *ci, *co;
  ci = (uint8_t *)data;
  co = (uint8_t *)&dVal;
  co[0] = ci[3];
  co[1] = ci[2];
  co[2] = ci[1];
  co[3] = ci[0];
  co[4] = ci[7];
  co[5] = ci[6];
  co[6] = ci[5];
  co[7] = ci[4];
#else /* __BYTE_ORDER == __BIG_ENDIAN && __FLOAT_WORD_ORER == __LITTLE_ENDIAN */
  uint8_t *ci, *co;
  ci = (uint8_t *)data;
  co = (uint8_t *)&dVal;
  co[0] = ci[4];
  co[1] = ci[5];
  co[2] = ci[6];
  co[3] = ci[7];
  co[4] = ci[0];
  co[5] = ci[1];
  co[6] = ci[2];
  co[7] = ci[3];
#endif
#endif
	return dVal;
}

int32_t AMF_DecodeBoolean(const char *data) {
	return *data != 0;
}

char*
AMF_EncodeInt16(char *output, char *outend, short nVal) {
	if (output + 2 > outend)
		return NULL;

	output[1] = nVal & 0xff;
	output[0] = nVal >> 8;
	return output + 2;
}

char*
AMF_EncodeInt24(char *output, char *outend, int32_t nVal) {
	if (output + 3 > outend)
		return NULL;

	output[2] = nVal & 0xff;
	output[1] = nVal >> 8;
	output[0] = nVal >> 16;
	return output + 3;
}

char*
AMF_EncodeInt32(char *output, char *outend, int32_t nVal) {
	if (output + 4 > outend)
		return NULL;

	output[3] = nVal & 0xff;
	output[2] = nVal >> 8;
	output[1] = nVal >> 16;
	output[0] = nVal >> 24;
	return output + 4;
}

char*
AMF_EncodeString(char *output, char *outend, const AVal *bv) {
	if ((bv->av_len < 65536 && output + 1 + 2 + bv->av_len > outend)
			|| output + 1 + 4 + bv->av_len > outend)
		return NULL;

	if (bv->av_len < 65536) {
		*output++ = AMF_STRING;

		output = AMF_EncodeInt16(output, outend, bv->av_len);
	} else {
		*output++ = AMF_LONG_STRING;

		output = AMF_EncodeInt32(output, outend, bv->av_len);
	}
	memcpy(output, bv->av_val, bv->av_len);
	output += bv->av_len;

	return output;
}

char*
AMF_EncodeNumber(char *output, char *outend, double dVal) {
	if (output + 1 + 8 > outend)
		return NULL;

	*output++ = AMF_NUMBER; /* type: Number */

#if __FLOAT_WORD_ORDER == __BYTE_ORDER
#if __BYTE_ORDER == __BIG_ENDIAN
  memcpy(output, &dVal, 8);
#elif __BYTE_ORDER == __LITTLE_ENDIAN
	{
		uint8_t *ci, *co;
		ci = (uint8_t*) &dVal;
		co = (uint8_t*) output;
		co[0] = ci[7];
		co[1] = ci[6];
		co[2] = ci[5];
		co[3] = ci[4];
		co[4] = ci[3];
		co[5] = ci[2];
		co[6] = ci[1];
		co[7] = ci[0];
	}
#endif
#else
#if __BYTE_ORDER == __LITTLE_ENDIAN	/* __FLOAT_WORD_ORER == __BIG_ENDIAN */
  {
    uint8_t *ci, *co;
    ci = (uint8_t *)&dVal;
    co = (uint8_t *)output;
    co[0] = ci[3];
    co[1] = ci[2];
    co[2] = ci[1];
    co[3] = ci[0];
    co[4] = ci[7];
    co[5] = ci[6];
    co[6] = ci[5];
    co[7] = ci[4];
  }
#else /* __BYTE_ORDER == __BIG_ENDIAN && __FLOAT_WORD_ORER == __LITTLE_ENDIAN */
  {
    uint8_t *ci, *co;
    ci = (uint8_t *)&dVal;
    co = (uint8_t *)output;
    co[0] = ci[4];
    co[1] = ci[5];
    co[2] = ci[6];
    co[3] = ci[7];
    co[4] = ci[0];
    co[5] = ci[1];
    co[6] = ci[2];
    co[7] = ci[3];
  }
#endif
#endif

	return output + 8;
}

char*
AMF_EncodeBoolean(char *output, char *outend, int32_t bVal) {
	if (output + 2 > outend)
		return NULL;

	*output++ = AMF_BOOLEAN;

	*output++ = bVal ? 0x01 : 0x00;

	return output;
}

char*
AMF_EncodeNamedString(char *output, char *outend, const AVal *strName,
		const AVal *strValue) {
	if (output + 2 + strName->av_len > outend)
		return NULL;
	output = AMF_EncodeInt16(output, outend, strName->av_len);

	memcpy(output, strName->av_val, strName->av_len);
	output += strName->av_len;

	return AMF_EncodeString(output, outend, strValue);
}

char*
AMF_EncodeNamedNumber(char *output, char *outend, const AVal *strName,
		double dVal) {
	if (output + 2 + strName->av_len > outend)
		return NULL;
	output = AMF_EncodeInt16(output, outend, strName->av_len);

	memcpy(output, strName->av_val, strName->av_len);
	output += strName->av_len;

	return AMF_EncodeNumber(output, outend, dVal);
}

char*
AMF_EncodeNamedBoolean(char *output, char *outend, const AVal *strName,
		int32_t bVal) {
	if (output + 2 + strName->av_len > outend)
		return NULL;
	output = AMF_EncodeInt16(output, outend, strName->av_len);

	memcpy(output, strName->av_val, strName->av_len);
	output += strName->av_len;

	return AMF_EncodeBoolean(output, outend, bVal);
}

void AMFProp_GetName(AMFObjectProperty *prop, AVal *name) {
	*name = prop->p_name;
}

void AMFProp_SetName(AMFObjectProperty *prop, AVal *name) {
	prop->p_name = *name;
}

AMFDataType AMFProp_GetType(AMFObjectProperty *prop) {
	return prop->p_type;
}

double AMFProp_GetNumber(AMFObjectProperty *prop) {
	return prop->p_vu.p_number;
}

int32_t AMFProp_GetBoolean(AMFObjectProperty *prop) {
	return prop->p_vu.p_number != 0;
}

void AMFProp_GetString(AMFObjectProperty *prop, AVal *str) {
	if (prop->p_type == AMF_STRING)
		*str = prop->p_vu.p_aval;
	else
		*str = AV_empty;
}

void AMFProp_GetObject(AMFObjectProperty *prop, AMFObject *obj) {
	if (prop->p_type == AMF_OBJECT)
		*obj = prop->p_vu.p_object;
	else
		*obj = AMFObj_Invalid;
}

int32_t AMFProp_IsValid(AMFObjectProperty *prop) {
	return prop->p_type != AMF_INVALID;
}

char*
AMFProp_Encode(AMFObjectProperty *prop, char *pBuffer, char *pBufEnd) {
	if (prop->p_type == AMF_INVALID)
		return NULL;

	if (prop->p_type != AMF_NULL
			&& pBuffer + prop->p_name.av_len + 2 + 1 >= pBufEnd)
		return NULL;

	if (prop->p_type != AMF_NULL && prop->p_name.av_len) {
		*pBuffer++ = prop->p_name.av_len >> 8;
		*pBuffer++ = prop->p_name.av_len & 0xff;
		memcpy(pBuffer, prop->p_name.av_val, prop->p_name.av_len);
		pBuffer += prop->p_name.av_len;
	}

	switch (prop->p_type) {
	case AMF_NUMBER:
		pBuffer = AMF_EncodeNumber(pBuffer, pBufEnd, prop->p_vu.p_number);
		break;

	case AMF_BOOLEAN:
		pBuffer = AMF_EncodeBoolean(pBuffer, pBufEnd, prop->p_vu.p_number != 0);
		break;

	case AMF_STRING:
		pBuffer = AMF_EncodeString(pBuffer, pBufEnd, &prop->p_vu.p_aval);
		break;

	case AMF_NULL:
		if (pBuffer + 1 >= pBufEnd)
			return NULL;
		*pBuffer++ = AMF_NULL;
		break;

	case AMF_OBJECT:
		pBuffer = AMF_Encode(&prop->p_vu.p_object, pBuffer, pBufEnd);
		break;

	case AMF_ECMA_ARRAY:
		pBuffer = AMF_EncodeEcmaArray(&prop->p_vu.p_object, pBuffer, pBufEnd);
		break;

	case AMF_STRICT_ARRAY:
		pBuffer = AMF_EncodeArray(&prop->p_vu.p_object, pBuffer, pBufEnd);
		break;

	default:
		Yang_Logs1(Yang_LOGERROR, "%s, invalid type. %d", __FUNCTION__,
				prop->p_type);
		pBuffer = NULL;
	};

	return pBuffer;
}

#define AMF3_INTEGER_MAX	268435455
#define AMF3_INTEGER_MIN	-268435456

int32_t AMF3ReadInteger(const char *data, int32_t *valp) {
	int32_t i = 0;
	int32_t val = 0;

	while (i <= 2) { /* handle first 3 bytes */
		if (data[i] & 0x80) { /* byte used */
			val <<= 7; /* shift up */
			val |= (data[i] & 0x7f); /* add bits */
			i++;
		} else {
			break;
		}
	}

	if (i > 2) { /* use 4th byte, all 8bits */
		val <<= 8;
		val |= data[3];

		/* range check */
		if (val > AMF3_INTEGER_MAX)
			val -= (1 << 29);
	} else { /* use 7bits of last unparsed byte (0xxxxxxx) */
		val <<= 7;
		val |= data[i];
	}

	*valp = val;

	return i > 2 ? 4 : i + 1;
}

int32_t AMF3ReadString(const char *data, AVal *str) {
	int32_t ref = 0;
	int32_t len;
	assert(str != 0);

	len = AMF3ReadInteger(data, &ref);
	data += len;

	if ((ref & 0x1) == 0) { /* reference: 0xxx */
		uint32_t refIndex = (ref >> 1);
		Yang_Logs1(Yang_LOGDEBUG,
				"%s, string reference, index: %d, not supported, ignoring!",
				__FUNCTION__, refIndex);
		str->av_val = NULL;
		str->av_len = 0;
		return len;
	} else {
		uint32_t nSize = (ref >> 1);

		str->av_val = (char*) data;
		str->av_len = nSize;

		return len + nSize;
	}
	return len;
}

int32_t AMF3Prop_Decode(AMFObjectProperty *prop, const char *pBuffer, int32_t nSize,
		int32_t bDecodeName) {
	int32_t nOriginalSize = nSize;
	AMF3DataType type;

	prop->p_name.av_len = 0;
	prop->p_name.av_val = NULL;

	if (nSize == 0 || !pBuffer) {
		Yang_Logs1(Yang_LOGDEBUG, "empty buffer/no buffer pointer!");
		return -1;
	}

	/* decode name */
	if (bDecodeName) {
		AVal name;
		int32_t nRes = AMF3ReadString(pBuffer, &name);

		if (name.av_len <= 0)
			return nRes;

		nSize -= nRes;
		if (nSize <= 0)
			return -1;
		prop->p_name = name;
		pBuffer += nRes;
	}

	/* decode */
	type = (AMF3DataType) *pBuffer++;
	nSize--;

	switch (type) {
	case AMF3_UNDEFINED:
	case AMF3_NULL:
		prop->p_type = AMF_NULL;
		break;
	case AMF3_FALSE:
		prop->p_type = AMF_BOOLEAN;
		prop->p_vu.p_number = 0.0;
		break;
	case AMF3_TRUE:
		prop->p_type = AMF_BOOLEAN;
		prop->p_vu.p_number = 1.0;
		break;
	case AMF3_INTEGER: {
		int32_t res = 0;
		int32_t len = AMF3ReadInteger(pBuffer, &res);
		prop->p_vu.p_number = (double) res;
		prop->p_type = AMF_NUMBER;
		nSize -= len;
		break;
	}
	case AMF3_DOUBLE:
		if (nSize < 8)
			return -1;
		prop->p_vu.p_number = AMF_DecodeNumber(pBuffer);
		prop->p_type = AMF_NUMBER;
		nSize -= 8;
		break;
	case AMF3_STRING:
	case AMF3_XML_DOC:
	case AMF3_XML: {
		int32_t len = AMF3ReadString(pBuffer, &prop->p_vu.p_aval);
		prop->p_type = AMF_STRING;
		nSize -= len;
		break;
	}
	case AMF3_DATE: {
		int32_t res = 0;
		int32_t len = AMF3ReadInteger(pBuffer, &res);

		nSize -= len;
		pBuffer += len;

		if ((res & 0x1) == 0) { /* reference */
			uint32_t nIndex = (res >> 1);
			Yang_Logs1(Yang_LOGDEBUG, "AMF3_DATE reference: %d, not supported!",
					nIndex);
		} else {
			if (nSize < 8)
				return -1;

			prop->p_vu.p_number = AMF_DecodeNumber(pBuffer);
			nSize -= 8;
			prop->p_type = AMF_NUMBER;
		}
		break;
	}
	case AMF3_OBJECT: {
		int32_t nRes = AMF3_Decode(&prop->p_vu.p_object, pBuffer, nSize, TRUE);
		if (nRes == -1)
			return -1;
		nSize -= nRes;
		prop->p_type = AMF_OBJECT;
		break;
	}
	case AMF3_ARRAY:
	case AMF3_BYTE_ARRAY:
	default:
		Yang_Logs1(Yang_LOGDEBUG,
				"%s - AMF3 unknown/unsupported datatype 0x%02x, @%p",
				__FUNCTION__, (uint8_t) (*pBuffer), pBuffer);
		return -1;
	}
	if (nSize < 0)
		return -1;

	return nOriginalSize - nSize;
}

int32_t AMFProp_Decode(AMFObjectProperty *prop, const char *pBuffer, int32_t nSize,
		int32_t bDecodeName) {
	int32_t nOriginalSize = nSize;
	int32_t nRes;

	prop->p_name.av_len = 0;
	prop->p_name.av_val = NULL;

	if (nSize == 0 || !pBuffer) {
		Yang_Logs1(Yang_LOGDEBUG, "%s: Empty buffer/no buffer pointer!",
				__FUNCTION__);
		return -1;
	}

	if (bDecodeName && nSize < 4) { /* at least name (length + at least 1 byte) and 1 byte of data */
		Yang_Logs1(Yang_LOGDEBUG,
				"%s: Not enough data for decoding with name, less than 4 bytes!",
				__FUNCTION__);
		return -1;
	}

	if (bDecodeName) {
		unsigned short nNameSize = AMF_DecodeInt16(pBuffer);
		if (nNameSize > nSize - 2) {
			Yang_Logs1(Yang_LOGDEBUG,
					"%s: Name size out of range: namesize (%d) > len (%d) - 2",
					__FUNCTION__, nNameSize, nSize);
			return -1;
		}

		AMF_DecodeString(pBuffer, &prop->p_name);
		nSize -= 2 + nNameSize;
		pBuffer += 2 + nNameSize;
	}

	if (nSize == 0) {
		return -1;
	}

	nSize--;

	prop->p_type = (AMFDataType) (*pBuffer++);
	switch (prop->p_type) {
	case AMF_NUMBER:
		if (nSize < 8)
			return -1;
		prop->p_vu.p_number = AMF_DecodeNumber(pBuffer);
		nSize -= 8;
		break;
	case AMF_BOOLEAN:
		if (nSize < 1)
			return -1;
		prop->p_vu.p_number = (double) AMF_DecodeBoolean(pBuffer);
		nSize--;
		break;
	case AMF_STRING: {
		unsigned short nStringSize = AMF_DecodeInt16(pBuffer);

		if (nSize < (long) nStringSize + 2)
			return -1;
		AMF_DecodeString(pBuffer, &prop->p_vu.p_aval);
		nSize -= (2 + nStringSize);
		break;
	}
	case AMF_OBJECT: {
		int32_t nRes = AMF_Decode(&prop->p_vu.p_object, pBuffer, nSize, TRUE);
		if (nRes == -1)
			return -1;
		nSize -= nRes;
		break;
	}
	case AMF_MOVIECLIP: {
		Yang_Logs1(Yang_LOGERROR, "AMF_MOVIECLIP reserved!");
		return -1;
		break;
	}
	case AMF_NULL:
	case AMF_UNDEFINED:
	case AMF_UNSUPPORTED:
		prop->p_type = AMF_NULL;
		break;
	case AMF_REFERENCE: {
		Yang_Logs1(Yang_LOGERROR, "AMF_REFERENCE not supported!");
		return -1;
		break;
	}
	case AMF_ECMA_ARRAY: {
		nSize -= 4;

		/* next comes the rest, mixed array has a final 0x000009 mark and names, so its an object */

		nRes = AMF_Decode(&prop->p_vu.p_object, pBuffer + 4, nSize, TRUE);
		//printf("\n%d....................AMF_ECMA_ARRAY\n",nRes);
		if (nRes == -1)
			return -1;
		nSize -= nRes;
		break;
	}
	case AMF_OBJECT_END: {
		return -1;
		break;
	}
	case AMF_STRICT_ARRAY: {
		uint32_t  nArrayLen = AMF_DecodeInt32(pBuffer);
		nSize -= 4;

		nRes = AMF_DecodeArray(&prop->p_vu.p_object, pBuffer + 4, nSize,
				nArrayLen, FALSE);
		if (nRes == -1)
			return -1;
		nSize -= nRes;
		break;
	}
	case AMF_DATE: {
		Yang_Logs1(Yang_LOGDEBUG, "AMF_DATE");

		if (nSize < 10)
			return -1;

		prop->p_vu.p_number = AMF_DecodeNumber(pBuffer);
		prop->p_UTCoffset = AMF_DecodeInt16(pBuffer + 8);

		nSize -= 10;
		break;
	}
	case AMF_LONG_STRING:
	case AMF_XML_DOC: {
		uint32_t  nStringSize = AMF_DecodeInt32(pBuffer);
		if (nSize < (long) nStringSize + 4)
			return -1;
		AMF_DecodeLongString(pBuffer, &prop->p_vu.p_aval);
		nSize -= (4 + nStringSize);
		if (prop->p_type == AMF_LONG_STRING)
			prop->p_type = AMF_STRING;
		break;
	}
	case AMF_RECORDSET: {
		Yang_Logs1(Yang_LOGERROR, "AMF_RECORDSET reserved!");
		return -1;
		break;
	}
	case AMF_TYPED_OBJECT: {
		Yang_Logs1(Yang_LOGERROR, "AMF_TYPED_OBJECT not supported!");
		return -1;
		break;
	}
	case AMF_AVMPLUS: {
		int32_t nRes = AMF3_Decode(&prop->p_vu.p_object, pBuffer, nSize, TRUE);
		if (nRes == -1)
			return -1;
		nSize -= nRes;
		prop->p_type = AMF_OBJECT;
		break;
	}
	default:
		Yang_Logs1(Yang_LOGDEBUG, "%s - unknown datatype 0x%02x, @%p",
				__FUNCTION__, prop->p_type, pBuffer - 1);
		return -1;
	}

	return nOriginalSize - nSize;
}

void AMFProp_Dump(AMFObjectProperty *prop) {
	char strRes[256];
	char str[256];
	AVal name;

	if (prop->p_type == AMF_INVALID) {
		Yang_Logs1(Yang_LOGDEBUG, "Property: INVALID");
		return;
	}

	if (prop->p_type == AMF_NULL) {
		Yang_Logs1(Yang_LOGDEBUG, "Property: NULL");
		return;
	}

	if (prop->p_name.av_len) {
		name = prop->p_name;
	} else {
		name.av_val = "no-name.";
		name.av_len = sizeof("no-name.") - 1;
	}
	if (name.av_len > 18)
		name.av_len = 18;

	snprintf(strRes, 255, "Name: %18.*s, ", name.av_len, name.av_val);

	if (prop->p_type == AMF_OBJECT) {
		Yang_Logs1(Yang_LOGDEBUG, "Property: <%sOBJECT>", strRes);
		AMF_Dump(&prop->p_vu.p_object);
		return;
	} else if (prop->p_type == AMF_ECMA_ARRAY) {
		Yang_Logs1(Yang_LOGDEBUG, "Property: <%sECMA_ARRAY>", strRes);
		AMF_Dump(&prop->p_vu.p_object);
		return;
	} else if (prop->p_type == AMF_STRICT_ARRAY) {
		Yang_Logs1(Yang_LOGDEBUG, "Property: <%sSTRICT_ARRAY>", strRes);
		AMF_Dump(&prop->p_vu.p_object);
		return;
	}

	switch (prop->p_type) {
	case AMF_NUMBER:
		snprintf(str, 255, "NUMBER:\t%.2f", prop->p_vu.p_number);
		break;
	case AMF_BOOLEAN:
		snprintf(str, 255, "BOOLEAN:\t%s",
				prop->p_vu.p_number != 0.0 ? "TRUE" : "FALSE");
		break;
	case AMF_STRING:
		snprintf(str, 255, "STRING:\t%.*s", prop->p_vu.p_aval.av_len,
				prop->p_vu.p_aval.av_val);
		break;
	case AMF_DATE:
		snprintf(str, 255, "DATE:\ttimestamp: %.2f, UTC offset: %d",
				prop->p_vu.p_number, prop->p_UTCoffset);
		break;
	default:
		snprintf(str, 255, "INVALID TYPE 0x%02x", (uint8_t) prop->p_type);
	}

	Yang_Logs1(Yang_LOGDEBUG, "Property: <%s%s>", strRes, str);
}
void AMFProp_Dump1(AMFObjectProperty *prop) {
	char strRes[256];
	char str[256];
	AVal name;

	if (prop->p_type == AMF_INVALID) {
		Yang_Logs1(Yang_LOGDEBUG, "Property: INVALID");
		return;
	}

	if (prop->p_type == AMF_NULL) {
		Yang_Logs1(Yang_LOGDEBUG, "Property: NULL");
		return;
	}

	if (prop->p_name.av_len) {
		name = prop->p_name;
	} else {
		name.av_val = "no-name.";
		name.av_len = sizeof("no-name.") - 1;
	}
	if (name.av_len > 18)
		name.av_len = 18;

	snprintf(strRes, 255, "Name: %18.*s, ", name.av_len, name.av_val);

	if (prop->p_type == AMF_OBJECT) {
		Yang_Logs1(Yang_LOGDEBUG, "Property: <%sOBJECT>", strRes);
		AMF_Dump(&prop->p_vu.p_object);
		return;
	} else if (prop->p_type == AMF_ECMA_ARRAY) {
		Yang_Logs1(Yang_LOGDEBUG, "Property: <%sECMA_ARRAY>", strRes);
		AMF_Dump(&prop->p_vu.p_object);
		return;
	} else if (prop->p_type == AMF_STRICT_ARRAY) {
		Yang_Logs1(Yang_LOGDEBUG, "Property: <%sSTRICT_ARRAY>", strRes);
		AMF_Dump(&prop->p_vu.p_object);
		return;
	}

	switch (prop->p_type) {
	case AMF_NUMBER:
		snprintf(str, 255, "NUMBER:\t%.2f", prop->p_vu.p_number);
		break;
	case AMF_BOOLEAN:
		snprintf(str, 255, "BOOLEAN:\t%s",
				prop->p_vu.p_number != 0.0 ? "TRUE" : "FALSE");
		break;
	case AMF_STRING:
		snprintf(str, 255, "STRING:\t%.*s", prop->p_vu.p_aval.av_len,
				prop->p_vu.p_aval.av_val);
		break;
	case AMF_DATE:
		snprintf(str, 255, "DATE:\ttimestamp: %.2f, UTC offset: %d",
				prop->p_vu.p_number, prop->p_UTCoffset);
		break;
	default:
		snprintf(str, 255, "INVALID TYPE 0x%02x", (uint8_t) prop->p_type);
	}

	Yang_Logs1(Yang_LOGERROR, "Property: <%s%s>", strRes, str);
}
void AMFProp_Reset(AMFObjectProperty *prop) {
	if (prop->p_type == AMF_OBJECT || prop->p_type == AMF_ECMA_ARRAY
			|| prop->p_type == AMF_STRICT_ARRAY)
		AMF_Reset(&prop->p_vu.p_object);
	else {
		prop->p_vu.p_aval.av_len = 0;
		prop->p_vu.p_aval.av_val = NULL;
	}
	prop->p_type = AMF_INVALID;
}

/* AMFObject */

char*
AMF_Encode(AMFObject *obj, char *pBuffer, char *pBufEnd) {
	int32_t i;

	if (pBuffer + 4 >= pBufEnd)
		return NULL;

	*pBuffer++ = AMF_OBJECT;

	for (i = 0; i < obj->o_num; i++) {
		char *res = AMFProp_Encode(&obj->o_props[i], pBuffer, pBufEnd);
		if (res == NULL) {
			Yang_Logs1(Yang_LOGERROR,
					"AMF_Encode - failed to encode property in index %d", i);
			break;
		} else {
			pBuffer = res;
		}
	}

	if (pBuffer + 3 >= pBufEnd)
		return NULL; /* no room for the end marker */

	pBuffer = AMF_EncodeInt24(pBuffer, pBufEnd, AMF_OBJECT_END);

	return pBuffer;
}

char*
AMF_EncodeEcmaArray(AMFObject *obj, char *pBuffer, char *pBufEnd) {
	int32_t i;

	if (pBuffer + 4 >= pBufEnd)
		return NULL;

	*pBuffer++ = AMF_ECMA_ARRAY;

	pBuffer = AMF_EncodeInt32(pBuffer, pBufEnd, obj->o_num);

	for (i = 0; i < obj->o_num; i++) {
		char *res = AMFProp_Encode(&obj->o_props[i], pBuffer, pBufEnd);
		if (res == NULL) {
			Yang_Logs1(Yang_LOGERROR,
					"AMF_Encode - failed to encode property in index %d", i);
			break;
		} else {
			pBuffer = res;
		}
	}

	if (pBuffer + 3 >= pBufEnd)
		return NULL; /* no room for the end marker */

	pBuffer = AMF_EncodeInt24(pBuffer, pBufEnd, AMF_OBJECT_END);

	return pBuffer;
}

char*
AMF_EncodeArray(AMFObject *obj, char *pBuffer, char *pBufEnd) {
	int32_t i;

	if (pBuffer + 4 >= pBufEnd)
		return NULL;

	*pBuffer++ = AMF_STRICT_ARRAY;

	pBuffer = AMF_EncodeInt32(pBuffer, pBufEnd, obj->o_num);

	for (i = 0; i < obj->o_num; i++) {
		char *res = AMFProp_Encode(&obj->o_props[i], pBuffer, pBufEnd);
		if (res == NULL) {
			Yang_Logs1(Yang_LOGERROR,
					"AMF_Encode - failed to encode property in index %d", i);
			break;
		} else {
			pBuffer = res;
		}
	}

	//if (pBuffer + 3 >= pBufEnd)
	//  return NULL;			/* no room for the end marker */

	//pBuffer = AMF_EncodeInt24(pBuffer, pBufEnd, AMF_OBJECT_END);

	return pBuffer;
}

int32_t AMF_DecodeArray(AMFObject *obj, const char *pBuffer, int32_t nSize,
		int32_t nArrayLen, int32_t bDecodeName) {
	int32_t nOriginalSize = nSize;
	int32_t bError = FALSE;

	obj->o_num = 0;
	obj->o_props = NULL;
	while (nArrayLen > 0) {
		AMFObjectProperty prop;
		int32_t nRes;
		nArrayLen--;

		if (nSize <= 0) {
			bError = TRUE;
			break;
		}
		nRes = AMFProp_Decode(&prop, pBuffer, nSize, bDecodeName);
		if (nRes == -1) {
			bError = TRUE;
			break;
		} else {
			nSize -= nRes;
			pBuffer += nRes;
			AMF_AddProp(obj, &prop);
		}
	}
	if (bError)
		return -1;

	return nOriginalSize - nSize;
}

int32_t AMF3_Decode(AMFObject *obj, const char *pBuffer, int32_t nSize, int32_t bAMFData) {
	int32_t nOriginalSize = nSize;
	int32_t ref;
	int32_t len;

	obj->o_num = 0;
	obj->o_props = NULL;
	if (bAMFData) {
		if (*pBuffer != AMF3_OBJECT)
			Yang_Logs1(Yang_LOGERROR,
					"AMF3 Object encapsulated in AMF stream does not start with AMF3_OBJECT!");
		pBuffer++;
		nSize--;
	}

	ref = 0;
	len = AMF3ReadInteger(pBuffer, &ref);
	pBuffer += len;
	nSize -= len;

	if ((ref & 1) == 0) { /* object reference, 0xxx */
		uint32_t objectIndex = (ref >> 1);

		Yang_Logs1(Yang_LOGDEBUG, "Object reference, index: %d", objectIndex);
	} else /* object instance */
	{
		int32_t classRef = (ref >> 1);

		AMF3ClassDef cd = { { 0, 0 } };
		AMFObjectProperty prop;

		if ((classRef & 0x1) == 0) { /* class reference */
			uint32_t classIndex = (classRef >> 1);
			Yang_Logs1(Yang_LOGDEBUG, "Class reference: %d", classIndex);
		} else {
			int32_t classExtRef = (classRef >> 1);
			int32_t i, cdnum;

			cd.cd_externalizable = (classExtRef & 0x1) == 1;
			cd.cd_dynamic = ((classExtRef >> 1) & 0x1) == 1;

			cdnum = classExtRef >> 2;

			/* class name */

			len = AMF3ReadString(pBuffer, &cd.cd_name);
			nSize -= len;
			pBuffer += len;

			/*std::string str = className; */

			Yang_Logs1(Yang_LOGDEBUG,
					"Class name: %s, externalizable: %d, dynamic: %d, classMembers: %d",
					cd.cd_name.av_val, cd.cd_externalizable, cd.cd_dynamic,
					cd.cd_num);

			for (i = 0; i < cdnum; i++) {
				AVal memberName;
				if (nSize <= 0) {

					return nOriginalSize;
				}
				len = AMF3ReadString(pBuffer, &memberName);
				Yang_Logs1(Yang_LOGDEBUG, "Member: %s", memberName.av_val);
				AMF3CD_AddProp(&cd, &memberName);
				nSize -= len;
				pBuffer += len;
			}
		}

		/* add as referencable object */

		if (cd.cd_externalizable) {
			int32_t nRes;
			AVal name = AVC("DEFAULT_ATTRIBUTE");

			Yang_Logs1(Yang_LOGDEBUG, "Externalizable, TODO check");

			nRes = AMF3Prop_Decode(&prop, pBuffer, nSize, FALSE);
			if (nRes == -1)
				Yang_Logs1(Yang_LOGDEBUG, "%s, failed to decode AMF3 property!",
						__FUNCTION__);
			else {
				nSize -= nRes;
				pBuffer += nRes;
			}

			AMFProp_SetName(&prop, &name);
			AMF_AddProp(obj, &prop);
		} else {
			int32_t nRes, i;
			for (i = 0; i < cd.cd_num; i++) /* non-dynamic */
			{
				if (nSize <= 0)
					goto invalid;
				nRes = AMF3Prop_Decode(&prop, pBuffer, nSize, FALSE);
				if (nRes == -1)
					Yang_Logs1(Yang_LOGDEBUG,
							"%s, failed to decode AMF3 property!",
							__FUNCTION__);

				AMFProp_SetName(&prop, AMF3CD_GetProp(&cd, i));
				AMF_AddProp(obj, &prop);

				pBuffer += nRes;
				nSize -= nRes;
			}
			if (cd.cd_dynamic) {
				int32_t len = 0;

				do {
					if (nSize <= 0)
						goto invalid;
					nRes = AMF3Prop_Decode(&prop, pBuffer, nSize, TRUE);
					AMF_AddProp(obj, &prop);

					pBuffer += nRes;
					nSize -= nRes;

					len = prop.p_name.av_len;
				} while (len > 0);
			}
		}
		Yang_Logs1(Yang_LOGDEBUG, "class object!");
	}
	invalid: Yang_Logs1(Yang_LOGDEBUG, "%s, invalid class encoding!",
			__FUNCTION__);

	return nOriginalSize - nSize;
}

int32_t AMF_Decode(AMFObject *obj, const char *pBuffer, int32_t nSize,int32_t bDecodeName) {
	int32_t nOriginalSize = nSize;
	int32_t bError = FALSE; /* if there is an error while decoding - try to at least find the end mark AMF_OBJECT_END */

	obj->o_num = 0;
	obj->o_props = NULL;
	while (nSize > 0) {
		AMFObjectProperty prop;
		int32_t nRes;

		if (nSize >= 3 && AMF_DecodeInt24(pBuffer) == AMF_OBJECT_END) {
			nSize -= 3;
			bError = FALSE;
			break;
		}

		if (bError) {
			Yang_Logs1(Yang_LOGERROR,
					"DECODING ERROR, IGNORING BYTES UNTIL NEXT KNOWN PATTERN!");
			nSize--;
			pBuffer++;
			continue;
		}

		nRes = AMFProp_Decode(&prop, pBuffer, nSize, bDecodeName);
		//printf("\nnRes====================%d\n",nRes);
		if (nRes == -1) {
			bError = TRUE;
			break;
		} else {
			nSize -= nRes;
			if (nSize < 0) {
				bError = TRUE;
				break;
			}
			pBuffer += nRes;
			AMF_AddProp(obj, &prop);
		}
	}

	if (bError)
		return -1;

	return nOriginalSize - nSize;
}

void AMF_AddProp(AMFObject *obj, const AMFObjectProperty *prop) {
	if (!(obj->o_num & 0x0f))
		obj->o_props = (struct AMFObjectProperty*) realloc(obj->o_props,
				(obj->o_num + 16) * sizeof(AMFObjectProperty));
	memcpy(&obj->o_props[obj->o_num++], prop, sizeof(AMFObjectProperty));
}

int32_t AMF_CountProp(AMFObject *obj) {
	return obj->o_num;
}

AMFObjectProperty*
AMF_GetProp(AMFObject *obj, const AVal *name, int32_t nIndex) {
	if (nIndex >= 0) {
		if (nIndex < obj->o_num)
			return &obj->o_props[nIndex];
	} else {
		int32_t n;
		for (n = 0; n < obj->o_num; n++) {
			if (AVMATCH(&obj->o_props[n].p_name, name))
				return &obj->o_props[n];
		}
	}

	return (AMFObjectProperty*) &AMFProp_Invalid;
}

void AMF_Dump(AMFObject *obj) {
	int32_t n;
	Yang_Logs1(Yang_LOGDEBUG, "(object begin)");
	for (n = 0; n < obj->o_num; n++) {
		AMFProp_Dump(&obj->o_props[n]);
	}
	Yang_Logs1(Yang_LOGDEBUG, "(object end)");
}
void AMF_Dump1(AMFObject *obj) {
	int32_t n;
	Yang_Logs1(Yang_LOGERROR, "(object begin)");
	for (n = 0; n < obj->o_num; n++) {
		AMFProp_Dump1(&obj->o_props[n]);
	}
	Yang_Logs1(Yang_LOGERROR, "(object end)");
}
void AMF_Reset(AMFObject *obj) {
	int32_t n;
	for (n = 0; n < obj->o_num; n++) {
		AMFProp_Reset(&obj->o_props[n]);
	}
	free(obj->o_props);
	obj->o_props = NULL;
	obj->o_num = 0;
}

/* AMF3ClassDefinition */

void AMF3CD_AddProp(AMF3ClassDef *cd, AVal *prop) {
	if (!(cd->cd_num & 0x0f))
		cd->cd_props = (AVal*) realloc(cd->cd_props,
				(cd->cd_num + 16) * sizeof(AVal));
	cd->cd_props[cd->cd_num++] = *prop;
}

AVal*
AMF3CD_GetProp(AMF3ClassDef *cd, int32_t nIndex) {
	if (nIndex >= cd->cd_num)
		return (AVal*) &AV_empty;
	return &cd->cd_props[nIndex];
}
