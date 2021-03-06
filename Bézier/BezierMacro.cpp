#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cmath>
#include <string>

#include <ldata.h>
#include <unistd.h> //getcwd

#define EXCLUDE_LEDIT_LEGACY_UPI //This statement make the C language macros, which are now superseded by C++ functions, unavailable.

#define BEZIER_PARAM 0.3
#define MAX_POLYGON_SIZE 5000
#define WIDTH 0.2
#define ANGLE_LIMIT 1.5

extern "C" {
    #include "DubinsPoint.hpp"

    #include "DubinsPoint.cpp"

    long Round0or5ToLong(double);
    long RoundToLong(double);
    void BezierMacro(void);
	int UPI_Entry_Point(void);
}

long Round0or5ToLong(double val)
{
	double tmpFloat = val;
	long tmpInt = 0;
	if(val >= 0)
		tmpFloat = (double)(tmpFloat + 2.5)/5.0;
	else
		tmpFloat = (double)(tmpFloat - 2.5)/5.0;
	tmpInt = (long)tmpFloat; //delete the digits after the '.'
	tmpInt = tmpInt*5;
	return tmpInt;
}

long RoundToLong(double value)
{
    return (long)(value+0.5);
}

void BezierMacro()
{
    DubinsPoint start, end;
    LPoint controlStart, controlEnd;

    LCell	pCell	=	LCell_GetVisible();
	LFile	pFile	=	LCell_GetFile(pCell);
    LLayer pLayer;

    char strLayer[MAX_LAYER_NAME];

    double xStart, yStart, xEnd, yEnd;
    double angleStart, angleEnd;
    double coef;
    double distX, distY;

    
    LPoint curve_arr[MAX_POLYGON_SIZE];
    int nbPointsCurve = 0;
    LPoint point_arr[MAX_POLYGON_SIZE];
    int nbPoints = 0;

    double t;
    int i = 0;
    int j = 0;
    double x, y;
    double angle, angle1, angle2;
    LPoint save1, save2, save3, save4;

    double guideWidth = LFile_MicronsToIntU(pFile, WIDTH);

    start.SetPoint(0,0,pFile);
    start.SetAngleDegre(0);
    end.SetPoint(10,10,pFile);
    end.SetAngleDegre(90);

    xStart = start.GetPoint().x;
    yStart = start.GetPoint().y;
    xEnd = end.GetPoint().x;
    yEnd = end.GetPoint().y;
    angleStart = start.GetAngleRadian();
    angleEnd = end.GetAngleRadian();

    coef = 1 - BEZIER_PARAM;

    if(xStart == xEnd || yStart == yEnd)
    {
        LDialog_AlertBox( "Start end end point has the same X or Y, unable to generate a Bezier curve" );
        return;
    }

    strcpy(strLayer, "WGUIDE"); //preloaded text in the dialog box
	if ( LDialog_InputBox("Layer", "Enter name of the layer in which the polygon will be loaded", strLayer) == 0)
		return;
	else
        pLayer = LLayer_Find(pFile, strLayer);

    strcpy(strLayer, "1");
    if ( LDialog_InputBox("Guide width", "Select the width of the guide (in microns)", strLayer) == 0)
        return;
    else
        guideWidth = LFile_MicronsToIntU(pFile, atof(strLayer));

LUpi_LogMessage(LFormat("BEGIN CREATING CURVE\n"));

    distX = xEnd - xStart;
    distY = yEnd - yStart;

    controlStart.x = (LCoord) ( xStart + distX * coef * cos(angleStart) );
    controlStart.y = (LCoord) ( yStart + distY * coef * sin(angleStart) );
    controlEnd.x = (LCoord) ( xEnd + distX * coef * cos(angleEnd + M_PI) );
    controlEnd.y = (LCoord) ( yEnd + distY * coef * sin(angleEnd + M_PI) );

    //construct the curve
    curve_arr[nbPointsCurve] = LPoint_Set( xStart, yStart );
    nbPointsCurve = nbPointsCurve + 1;
    for(t=0.0005; t<1; t=t+0.0005)
    {
        x = xStart*pow((1-t),3) + 3*controlStart.x*pow((1-t),2)*t + 3*controlEnd.x*(1-t)*pow(t,2) + xEnd*pow(t,3);
        y = yStart*pow((1-t),3) + 3*controlStart.y*pow((1-t),2)*t + 3*controlEnd.y*(1-t)*pow(t,2) + yEnd*pow(t,3);
        curve_arr[nbPointsCurve] = LPoint_Set( RoundToLong(x), RoundToLong(y) );
        nbPointsCurve = nbPointsCurve + 1;
    }
    curve_arr[nbPointsCurve] = LPoint_Set( xEnd, yEnd );
    nbPointsCurve = nbPointsCurve + 1;

    //construct the guide from the curve
    point_arr[nbPoints] = LPoint_Set((LCoord)(xStart + sin(angleStart) * guideWidth / 2.0) , (LCoord)(yStart - cos(angleStart) * guideWidth / 2.0));
    save1 = point_arr[nbPoints];
    nbPoints = nbPoints + 1;
    for(i=1; i<nbPointsCurve-1; i++)
    {
        angle = atan2( curve_arr[i+1].y-curve_arr[i].y , curve_arr[i+1].x-curve_arr[i].x );
        point_arr[nbPoints] = LPoint_Set((LCoord)(curve_arr[i].x + sin(angle) * guideWidth / 2.0) , (LCoord)(curve_arr[i].y - cos(angle) * guideWidth / 2.0));
        nbPoints = nbPoints + 1;
    }
    point_arr[nbPoints] = LPoint_Set((LCoord)(xEnd + sin(angleEnd) * guideWidth / 2.0) , (LCoord)(yEnd - cos(angleEnd) * guideWidth / 2.0));
    save2 = point_arr[nbPoints];
    nbPoints = nbPoints + 1;

    point_arr[nbPoints] = LPoint_Set((LCoord)(xEnd + sin(angleEnd + M_PI) * guideWidth / 2.0) , (LCoord)(yEnd - cos(angleEnd + M_PI) * guideWidth / 2.0));
    save3 = point_arr[nbPoints];
    nbPoints = nbPoints + 1;
    for(i=nbPointsCurve-2; i>=1; i--)
    {
        angle = atan2( curve_arr[i-1].y-curve_arr[i].y , curve_arr[i-1].x-curve_arr[i].x );
        point_arr[nbPoints] = LPoint_Set((LCoord)(curve_arr[i].x + sin(angle) * guideWidth / 2.0) , (LCoord)(curve_arr[i].y - cos(angle) * guideWidth / 2.0));
        nbPoints = nbPoints + 1;
    }
    point_arr[nbPoints] = LPoint_Set((LCoord)(xStart + sin(angleStart + M_PI) * guideWidth / 2.0) , (LCoord)(yStart - cos(angleStart + M_PI) * guideWidth / 2.0));
    save4 = point_arr[nbPoints];
    nbPoints = nbPoints + 1;

    LUpi_LogMessage(LFormat("nbPoints %d\n",nbPoints));

    //delete the points that intersect with the polygon
    j=1;
    while(j != 0)
    {
        j=0;
        for(i=0; i<nbPoints; i++)
        {
            if((save1.x==point_arr[i].x && save1.y==point_arr[i].y) || (save2.x==point_arr[i].x && save2.y==point_arr[i].y) || (save3.x==point_arr[i].x && save3.y==point_arr[i].y) || (save4.x==point_arr[i].x && save4.y==point_arr[i].y))
                continue;
            if(i==0)
                angle1 = atan2(point_arr[0].y-point_arr[nbPoints-1].y,point_arr[0].x-point_arr[nbPoints-1].x) - M_PI;
            else
                angle1 = atan2(point_arr[i].y-point_arr[i-1].y,point_arr[i].x-point_arr[i-1].x) - M_PI;
            if(i==nbPoints-1)
                angle2 = atan2(point_arr[nbPoints-1].y-point_arr[0].y,point_arr[nbPoints-1].x-point_arr[0].x);
            else
                angle2 = atan2(point_arr[i].y-point_arr[i+1].y,point_arr[i].x-point_arr[i+1].x);
            angle = angle2 - angle1;
            angle = fmod(angle, 2*M_PI);
            while(angle < 0)
                angle = angle + 2*M_PI;
//LUpi_LogMessage(LFormat("i %d\n",i));
            if( (angle > M_PI - ANGLE_LIMIT && angle < M_PI +ANGLE_LIMIT) ) //if not in the limit range
            {
                //point_arr[i]=point_arr[(i+1)%nbPoints];
                //point_arr[(i+1)%nbPoints]=point_arr[i];
                for(j=i; j<nbPoints; j++)
                    point_arr[j]=point_arr[(j+1)%nbPoints];
                nbPoints = nbPoints - 1;
                j=1;
            }                
        }
    }
    
    LUpi_LogMessage(LFormat("nbPoints %d\n",nbPoints));

//    curve_arr[nbPointsCurve] = LPoint_Set( 1, 0 );
//    nbPointsCurve = nbPointsCurve + 1;
    LPolygon_New( pCell, pLayer, point_arr, nbPoints );
}

int UPI_Entry_Point(void)
{
	LMacro_BindToMenuAndHotKey_v9_30("Tools", "F2" /*hotkey*/, "Bezier Macro", "BezierMacro", NULL /*hotkey category*/);
	return 1;
}