/*
 * pd_helios
 * © 2019 Tim Redfern
 * licenced under the LGPL v3.0
 * see LICENSE
 */

 #include "pd_helios.h"

#define _USE_MATH_DEFINES
 
#include <cmath>

#define RAD_2_DEG 180.0 / M_PI

using namespace std;

static float getLineDegreesAtIndex(vector <point> &line,int i){
	//return angle between this point and the next
	if ((i<1)||(i>(line.size()-2))){
		return 0.0f;
	}
	//https://stackoverflow.com/questions/14066933/direct-way-of-computing-clockwise-angle-between-2-vectors

	// dot product between [x1, y1] and [x2, y2]
	// dot = x1*x2 + y1*y2
	
	float dot = ((line[i].x-line[i-1].x)*(line[i+1].x-line[i].x)) + 
		((line[i].y-line[i-1].y)*(line[i+1].y-line[i].y));    

	// determinant det = x1*y2 - y1*x2

	float det = ((line[i].x-line[i-1].x)*(line[i+1].y-line[i].y)) + 
		((line[i].y-line[i-1].y)*(line[i+1].x-line[i].x));       
	
	return atan2(det, dot)* RAD_2_DEG;  // atan2(y, x) or atan2(sin, cos)
}

int Helios::draw(std::vector <point> &points){
	line=points;
    return (raw_mode==0)? draw() : draw_raw();
}

int Helios::draw_raw(){

    int xoffs=output_centre.x;
    int yoffs=output_centre.y;
    int pollcount;

    vector <HeliosPoint> points;
    vector <HeliosPoint> offsetpoints;
  
    int flags = (start_immediately * HELIOS_FLAGS_START_IMMEDIATELY)
              | (single_mode * HELIOS_FLAGS_SINGLE_MODE)
              | (dont_block * HELIOS_FLAGS_DONT_BLOCK);
    int i;
    for (i=0;i<line.size();i++){
        points.push_back(HeliosPoint{
                (uint16_t)(line[i].x+xoffs),
                (uint16_t)(line[i].y+yoffs),
                (uint8_t)(line[i].r*intensity/255.0),
                (uint8_t)(line[i].g*intensity/255.0),
                (uint8_t)(line[i].b*intensity/255.0),
                (uint8_t)255}
                );
    }
    for (auto& p:points){ //avoid problems with excessive scale
        p.x=min((uint16_t)0xfff,p.x);
        p.y=min((uint16_t)0xfff,p.y);
    }

    if (blank_offset != 0) {
        for (int i=0; i < points.size(); i++) {
            int offidx = (i + blank_offset) % points.size();
            offsetpoints.push_back(HeliosPoint {
               points[i].x,
               points[i].y,
               points[offidx].r,
               points[offidx].g,
               points[offidx].b
            });
         }
        points = offsetpoints;
    }

	if (device!=HELIOS_NODEVICE){
        if (enabled){
            pollcount = 0;
            for (pollcount=0; pollcount < max_status_poll; pollcount++) {
                if (dac.GetStatus(device) == 1) {
                    break;
                }
            }
	        int ret=dac.WriteFrame(device, pps, flags, &points[0], min(HELIOS_MAX_POINTS,(int)points.size()));

            if (framedump_requested) {
                dump_frame(points);
            }
	        if (ret==HELIOS_SUCCESS){
                return points.size();
	        }
	        return ret;
	    }
	}

	return -2;
}


int Helios::draw(){

    int xoffs=output_centre.x;
    int yoffs=output_centre.y;
    int pollcount;
    //save data
    vector <HeliosPoint> points;
    vector <HeliosPoint> offsetpoints;

    int flags = (start_immediately * HELIOS_FLAGS_START_IMMEDIATELY)
              | (single_mode * HELIOS_FLAGS_SINGLE_MODE)
              | (dont_block * HELIOS_FLAGS_DONT_BLOCK);
    //insert blank points to get laser to shape starting point
    for (int l=0;l<blank_num;l++){
        points.push_back(HeliosPoint{
            (uint16_t)(line[0].x+xoffs),
            (uint16_t)(line[0].y+yoffs),
            (uint8_t)(0),
            (uint8_t)(0),
            (uint8_t)(0),
            (uint8_t)255}
            );
    }


    //step through line points
    int i;
    for (i=0;i<line.size();i++){
        	            
        float angle=abs(getLineDegreesAtIndex(line,i));
        //cout << "point "<<i<<" - "<<angle<<" degrees, inserting "<<(angle>max_angle?((angle/180)*blank_num):0)<<" dwell points"<<endl;
            
        //insert dwell points to wait on a corner for laser to change direction
        int num_dwellpoints=0;

        if (i==line.size()-1) {
        	num_dwellpoints=blank_num/2;
        }
        	
        if (angle>max_angle) {
        	num_dwellpoints=((angle/180)*blank_num);
        }

    	for (int l=0;l<num_dwellpoints;l++){
            points.push_back(HeliosPoint{
                (uint16_t)(line[i].x+xoffs),
                (uint16_t)(line[i].y+yoffs),
                (uint8_t)(line[i].r*intensity/255.0),
                (uint8_t)(line[i].g*intensity/255.0),
                (uint8_t)(line[i].b*intensity/255.0),
                (uint8_t)255}
                );
        }

        if (i+1<line.size()){
	        float dist=point(line[i]).distance(point(line[i+1]));
	        int inserted=0;
	        for (float j=0;j<dist;j+=subdivide){
	        	inserted++;
	            //draw way points
	            float amt=j/dist;
	            points.push_back(HeliosPoint{
	                (uint16_t)((line[i].x*(1.0-amt))+(line[i+1].x*amt)+xoffs),
	                (uint16_t)((line[i].y*(1.0-amt))+(line[i+1].y*amt)+yoffs),
	                (uint8_t)((((line[i].r*(1.0-amt))+(line[i+1].r*amt))*intensity)/255.0),
	                (uint8_t)((((line[i].g*(1.0-amt))+(line[i+1].g*amt))*intensity)/255.0),
	                (uint8_t)((((line[i].b*(1.0-amt))+(line[i+1].b*amt))*intensity)/255.0),
	                (uint8_t)255}
	            );
	        }
	        //cout << "segment "<<i<<" - "<<dist<<" length, inserted "<<inserted<<" points"<<endl;
	    }
    }

    if (blank_offset != 0) {
        for (int i=0; i < points.size(); i++) {
            int offidx = (i + blank_offset) % points.size();
            offsetpoints.push_back(HeliosPoint {
               points[i].x,
               points[i].y,
               points[offidx].r,
               points[offidx].g,
               points[offidx].b
            });
         }
        points = offsetpoints;
    }

    for (auto& p:points){ //avoid problems with excessive scale
        p.x=min((uint16_t)0xfff,p.x);
        p.y=min((uint16_t)0xfff,p.y);
    }

	if (device!=HELIOS_NODEVICE){
    	if (enabled){
            pollcount = 0;
            for (pollcount=0; pollcount < max_status_poll; pollcount++) {
                if (dac.GetStatus(device) == 1) {
                    break;
                }
            }
            if (framedump_requested) {
                dump_frame(points);
            }
            if (points.size() > 4096) {
                cout << "Too many points: " << points.size() << endl;
                return HELIOS_ERROR_TOO_MANY_POINTS;
            }
	        int ret=dac.WriteFrame(device, pps, flags, &points[0], min(HELIOS_MAX_POINTS,(int)points.size()));
	        if (ret==HELIOS_SUCCESS){
	        	return points.size();
	        }
	        return ret;
	    }
	}

	return -2;
}


void Helios::dump_frame(std::vector <HeliosPoint> points) {
    std::cout << "frame: ";
    for (auto& p:points) {
        std::cout << "\t["
                  << (int)p.x-output_centre.x << ", "
                  << (int)p.y-output_centre.y << "] "
                  << "[" << (int)p.r << ", " << (int)p.g << ", " << (int)p.b << "]\n";
    }
    std::cout << std::endl;
    framedump_requested = 0;
}
