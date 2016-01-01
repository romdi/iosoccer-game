#include "cbase.h"
#include "ios_color_classifier.h"

inline double clamphue(double hue)
{
	hue = fmod(hue, M_PI * 2);

	if (hue < 0)
	{
		hue += M_PI * 2;
	}

	return hue;
}

struct RGB
{
	int R;
	int G;
	int B;
};

struct LCh
{
	double L;
	double C;
	double h;
};

struct HSL
{
	double H;
	double S;
	double L;
};


double toLabc(double c)
{
	if (c > 216 / 24389.0) return pow(c, 1 / 3.0);
	return c * (841 / 108.0) + (4 / 49.0);
}

void RGBtoLCh(const RGB &rgb, LCh &lch)
{
	double R = clamp(rgb.R, 0, 255) / 255.0;        //R from 0 to 255
	double G = clamp(rgb.G, 0, 255) / 255.0;        //G from 0 to 255
	double B = clamp(rgb.B, 0, 255) / 255.0;        //B from 0 to 255

	if (R > 0.04045)
		R = pow((R + 0.055) / 1.055, 2.4);
	else
		R = R / 12.92;

	if (G > 0.04045)
		G = pow((G + 0.055) / 1.055, 2.4);
	else
		G = G / 12.92;

	if (B > 0.04045)
		B = pow((B + 0.055) / 1.055, 2.4);
	else
		B = B / 12.92;

	double X = clamp(0.4124564 * R + 0.3575761 * G + 0.1804375 * B, 0, (31271 / 32902.0));
	double Y = clamp(0.2126729 * R + 0.7151522 * G + 0.0721750 * B, 0, 1);
	double Z = clamp(0.0193339 * R + 0.1191920 * G + 0.9503041 * B, 0, (35827 / 32902.0));

	double X_Lab = toLabc(X / (31271 / 32902.0)); // normalized standard observer D65.
	double Y_Lab = toLabc(Y / 1.0);
	double Z_Lab = toLabc(Z / (35827 / 32902.0));

	double L = clamp(116 * Y_Lab - 16, 0, 100);
	double a = clamp(500 * (X_Lab - Y_Lab), -12500 / 29.0, 12500 / 29.0);
	double b = clamp(200 * (Y_Lab - Z_Lab), -5000 / 29.0, 5000 / 29.0);

	double C = clamp(sqrt(a * a + b * b), 0, 4.64238345442629658e2); // 2500*sqrt(1 / 29.0)
	double h = clamphue(atan2(b, a));

	lch.L = L;
	lch.C = C;
	lch.h = h;
}

void RGBtoHSL(const RGB &rgb, HSL &hsl)
{
	double R = (rgb.R / 255.0);                     //RGB from 0 to 255
	double G = (rgb.G / 255.0);
	double B = (rgb.B / 255.0);

	double minVal = min(min(R, G), B);    //Min. value of RGB
	double maxVal = max(max(R, G), B);    //Max. value of RGB
	double deltaMax = maxVal - minVal;             //Delta RGB value

	double L = (maxVal + minVal) / 2;
	double H = 0;
	double S = 0;

	if (deltaMax == 0)                     //This is a gray, no chroma...
	{
		H = 0;                                //HSL results from 0 to 1
		S = 0;
	}
	else                                    //Chromatic data...
	{
		if (L < 0.5) S = deltaMax / (maxVal + minVal);
		else           S = deltaMax / (2 - maxVal - minVal);

		double deltaR = (((maxVal - R) / 6) + (deltaMax / 2)) / deltaMax;
		double deltaG = (((maxVal - G) / 6) + (deltaMax / 2)) / deltaMax;
		double deltaB = (((maxVal - B) / 6) + (deltaMax / 2)) / deltaMax;

		if (R == maxVal) H = deltaB - deltaG;
		else if (G == maxVal) H = (1 / 3.0) + deltaR - deltaB;
		else if (B == maxVal) H = (2 / 3.0) + deltaG - deltaR;

		if (H < 0) H += 1;
		if (H > 1) H -= 1;
	}

	hsl.H = H * 360;
	hsl.S = S * 100;
	hsl.L = L * 100;
}

// CIE Delta E 2000
// Note: maximum is about 158 for colors in the sRGB gamut.
double deltaE2000(RGB rgb1, RGB rgb2)
{
	LCh lch1, lch2;
	RGBtoLCh(rgb1, lch1);
	RGBtoLCh(rgb2, lch2);

	double avg_L = (lch1.L + lch2.L) * 0.5;
	double delta_L = lch2.L - lch1.L;

	double avg_C = (lch1.C + lch2.C) * 0.5;
	double delta_C = lch1.C - lch2.C;

	double avg_H = (lch1.h + lch2.h) * 0.5;

	if (abs(lch1.h - lch2.h) > M_PI)
	{
		avg_H += M_PI;
	}

	double delta_H = lch2.h - lch1.h;

	if (abs(delta_H) > M_PI)
	{
		if (lch2.h <= lch1.h) delta_H += M_PI * 2;
		else delta_H -= M_PI * 2;
	}

	delta_H = sqrt(lch1.C * lch2.C) * sin(delta_H) * 2;

	double T = 1
		- 0.17 * cos(avg_H - M_PI / 6.0)
		+ 0.24 * cos(avg_H * 2)
		+ 0.32 * cos(avg_H * 3 + M_PI / 30.0)
		- 0.20 * cos(avg_H * 4 - M_PI * 7 / 20.0);

	double SL = avg_L - 50;
	SL *= SL;
	SL = SL * 0.015 / sqrt(SL + 20) + 1;

	double SC = avg_C * 0.045 + 1;

	double SH = avg_C * T * 0.015 + 1;

	double delta_Theta = avg_H / 25.0 - M_PI * 11 / 180.0;
	delta_Theta = exp(delta_Theta * -delta_Theta) * (M_PI / 6.0);

	double RT = pow(avg_C, 7);
	RT = sqrt(RT / (RT + 6103515625)) * sin(delta_Theta) * -2; // 6103515625 = 25^7

	delta_L /= SL;
	delta_C /= SC;
	delta_H /= SH;

	return sqrt(delta_L * delta_L + delta_C * delta_C + delta_H * delta_H + RT * delta_C * delta_H);
}

color_class_t classify(const HSL &hsl)
{
	double hue = hsl.H;
	double sat = hsl.S;
	double lgt = hsl.L;

	if (lgt < 20)	return COLOR_CLASS_BLACK;
	if (lgt > 80)	return COLOR_CLASS_WHITE;

	if (sat < 25)	return COLOR_CLASS_GRAY;

	if (hue < 30)   return COLOR_CLASS_RED;
	if (hue < 90)   return COLOR_CLASS_YELLOW;
	if (hue < 150)  return COLOR_CLASS_GREEN;
	if (hue < 210)  return COLOR_CLASS_CYAN;
	if (hue < 270)  return COLOR_CLASS_BLUE;
	if (hue < 330)  return COLOR_CLASS_MAGENTA;
	else			return COLOR_CLASS_RED; // (hue >= 330)
}

color_class_t CColorClassifier::Classify(const Color &color)
{
	RGB rgbColor = { color.r(), color.g(), color.b() };
	HSL hslColor;
	RGBtoHSL(rgbColor, hslColor);
	color_class_t colorClass = classify(hslColor);

	return colorClass;
}