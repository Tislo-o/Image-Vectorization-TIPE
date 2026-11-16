// http://www.easyrgb.com/index.php?X=MATH&H=02#text2

extern float ref_X;
extern float ref_Y;
extern float ref_Z;

/***************************************************
 *  Name:        convertRGBtoXYZ
 *
 *  Returns:     Nothing
 *
 *  Parameters:  RGB values and XYZ references
 *
 *  Description: Given a color in RGB values converts  
 *               to the corresponding XYZ values
 *
 ***************************************************/
void convertRGBtoXYZ(unsigned char inR, unsigned char inG, unsigned char inB, float * outX, float * outY, float * outZ);


/***************************************************
 *  Name:        convertXYZtoLab
 *
 *  Returns:     Nothing
 *
 *  Parameters:  XYZ values and Lab references
 *
 *  Description: Given a color in XYZ values converts  
 *               to the corresponding Lab values
 *
 ***************************************************/
void convertXYZtoLab(float inX, float inY, float inZ, float * outL, float * outa, float * outb);

/***************************************************
 *  Name:        convertLabtoXYZ
 *
 *  Returns:     Nothing
 *
 *  Parameters:  Lab values and XYZ references
 *
 *  Description: Given a color in Lab values converts  
 *               to the corresponding XYZ values
 *
 ***************************************************/
void convertLabtoXYZ( float inL, float ina, float  inb, float * outX, float * outY, float * outZ);

/***************************************************
 *  Name:        convertXYZtoRGB
 *
 *  Returns:     Nothing
 *
 *  Parameters:  XYZ values and RGB references
 *
 *  Description: Given a color in XYZ values converts  
 *               to the corresponding RGB values
 *
 ***************************************************/
void convertXYZtoRGB(float inX, float inY, float inZ, unsigned char * outR, unsigned char * outG, unsigned char * outB);
//-----J'ai ajouté tout ce qui est en dessous:

void convertRGBtoLAB(unsigned char inR, unsigned char inG, unsigned char inB, float* outL, float* outa, float* outb);

void convertLabtoRGB(float inL, float ina, float inb, unsigned char* outR, unsigned char* outG, unsigned char* outB);
