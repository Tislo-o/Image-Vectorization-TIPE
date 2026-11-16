
#include "color_conversions.h"
#include <math.h>

float ref_X = 95.047;
float ref_Y = 100.0;
float ref_Z = 108.883;

void convertRGBtoXYZ(unsigned char inR, unsigned char inG, unsigned char inB, float * outX, float * outY, float * outZ) {
	float var_R = (inR / 255.0f); //R from 0 to 255
	float var_G = (inG / 255.0f); //G from 0 to 255
	float var_B = (inB / 255.0f); //B from 0 to 255

	if (var_R > 0.04045f)
		var_R = powf(( (var_R + 0.055f) / 1.055f), 2.4f);
	else 
		var_R = var_R / 12.92f;

	if (var_G > 0.04045f)
		var_G = powf(( (var_G + 0.055f) / 1.055f), 2.4f);
	else
		var_G = var_G / 12.92f;

	if (var_B > 0.04045f)
		var_B = powf(( (var_B + 0.055f) / 1.055f), 2.4f);
	else
		var_B = var_B / 12.92f;

	var_R = var_R * 100;
	var_G = var_G * 100;
	var_B = var_B * 100;

	//Observer. = 2°, Illuminant = D65
	*outX = var_R * 0.4124f + var_G * 0.3576f + var_B * 0.1805f;
	*outY = var_R * 0.2126f + var_G * 0.7152f + var_B * 0.0722f;
	*outZ = var_R * 0.0193f + var_G * 0.1192f + var_B * 0.9505f;
}


void convertXYZtoLab(float inX, float inY, float inZ, float * outL, float * outa, float * outb) {
	float var_X = (inX / ref_X); //ref_X = 95.047
	float var_Y = (inY / ref_Y); //ref_Y = 100.0
	float var_Z = (inZ / ref_Z); //ref_Z = 108.883

	if ( var_X > 0.008856 ) 
		var_X = powf(var_X , ( 1.0f/3 )); 
	else 
		var_X = ( 7.787 * var_X ) + ( 16.0f/116 );

	if ( var_Y > 0.008856 )
		var_Y = powf(var_Y , ( 1.0f/3 )); 
	else
	    var_Y = ( 7.787 * var_Y ) + ( 16.0f/116 );

	if ( var_Z > 0.008856 )
		var_Z = powf(var_Z , ( 1.0f/3 )); 
	else 
		var_Z = ( 7.787 * var_Z ) + ( 16.0f/116 );

	*outL = ( 116 * var_Y ) - 16;
	*outa = 500 * ( var_X - var_Y );
	*outb = 200 * ( var_Y - var_Z );
}


void convertLabtoXYZ( float inL, float ina, float  inb, float * outX, float * outY, float * outZ) {
	float var_Y = ( inL + 16.f ) / 116.f;
	float var_X = (ina/500.f) + var_Y;
	float var_Z = var_Y - (inb/200.f);

	if ( powf(var_Y,3.f) > 0.008856f ) 
		var_Y = powf(var_Y,3.f);
	else
		var_Y = ( var_Y - (16.f/116.f) ) / 7.787f;

	if ( powf(var_X,3.f) > 0.008856f ) 
		var_X = powf(var_X,3.f);
	else 
		var_X = ( var_X - (16.f/116.f) ) / 7.787f;
	
	if ( powf(var_Z,3.f) > 0.008856f )
		var_Z = powf(var_Z,3.f);
	else
		var_Z = ( var_Z - (16.f/116.f) ) / 7.787f;

	*outX = ref_X * var_X;     //ref_X =  95.047     Observer= 2°, Illuminant= D65
	*outY = ref_Y * var_Y;     //ref_Y = 100.000
	*outZ = ref_Z * var_Z;     //ref_Z = 108.883
}

unsigned char toByte(float v) {
	if (v < 0.0f) v = 0.0f;
	if (v > 1.0f) v = 1.0f;
	return (unsigned char)(v * 255.0f + 0.5f);
}

void convertXYZtoRGB(float inX, float inY, float inZ, unsigned char * outR, unsigned char * outG, unsigned char * outB) {
	float var_X = inX/100.f;
	float var_Y = inY/100.f;
	float var_Z = inZ/100.f;

	float var_R = var_X *  3.2406f + (var_Y * -1.5372f) + var_Z * (-0.4986f);
	float var_G = var_X * (-0.9689f) + var_Y *  1.8758f + var_Z *  0.0415f;
	float var_B = var_X *  0.0557f + var_Y * (-0.2040f) + var_Z *  1.0570f;

	if ( var_R > 0.0031308f )
		var_R = 1.055f * powf(var_R, ( 1.0f / 2.4f ) )  - 0.055f;
	else 
		var_R = 12.92f * var_R;

	if ( var_G > 0.0031308f ) 
		var_G = 1.055f * powf(var_G, ( 1.0f / 2.4f ) ) - 0.055f;
	else 
		var_G = 12.92f * var_G;

	if ( var_B > 0.0031308f )
		var_B = 1.055f * powf(var_B, ( 1.0f / 2.4f ) ) - 0.055f;
	else
		var_B = 12.92f * var_B;

	*outR = toByte(var_R);
	*outG = toByte(var_G);
	*outB = toByte(var_B);
}
//-----J'ai ajouté tout ce qui est en dessous:

void convertRGBtoLAB(unsigned char inR, unsigned char inG, unsigned char inB, float* outL, float* outa, float* outb) {
	float X, Y, Z;
	convertRGBtoXYZ(inR, inG, inB, &X, &Y, &Z);
	convertXYZtoLab(X, Y, Z, outL, outa, outb);
}

void convertLabtoRGB(float inL, float ina, float inb, unsigned char* outR, unsigned char* outG, unsigned char* outB) {
	float X, Y, Z;
	convertLabtoXYZ(inL, ina, inb, &X, &Y, &Z);
	convertXYZtoRGB(X, Y, Z, outR, outG, outB);
}
