/*
 * pd_helios
 * © 2019 Tim Redfern
 * licenced under the LGPL v3.0
 * see LICENSE
 */

#ifndef Helios_h
#define Helios_h
#define HELIOS_VERSION 0.2
#define HELIOS_NODEVICE -1

#define DEFAULT_SUBDIVIDE 15
#define DEFAULT_BLANK_NUM 8
#define DEFAULT_MAX_ANGLE 15.0f
#define DEFAULT_RAW_MODE 0
#define DEFAULT_MAX_STATUS_POLL 512

#include "sdk/HeliosDac.h"

#include <iostream>
#include <math.h>

//class that resembles a HeliosPoint but with floats for coords
class point {
public:
	point(){
		x=y=0;
		r=g=b=0;
	}
	point(float _x, float _y){
		x=_x;
		y=_y;
	}
	point(int _x, int _y){
		x=_x;
		y=_y;
	}
	point(float _x, float _y,uint8_t _r,uint8_t _g,uint8_t _b){
		x=_x;
		y=_y;
		r=_r;
		g=_g;
		b=_b;
	}
	point(const point &p){
		x=p.x;
		y=p.y;
		r=p.r;
		g=p.g;
		b=p.b;
	}
	point operator+ (const point& p){
		point np(x+p.x,y+p.y);
		return np;
	}
	point& operator+= (const point& p){
		x+=p.x;
		y+=p.y;
		return *this;
	}
	bool operator!=( const point&  p){
		return x==p.x&&y==p.y;
	}
    point& operator*= (const float f) {
        x *= f;
        y *= f;
        return *this;
    }
	float distance(const point&  p){
		return pow((pow(p.x-x,2)+pow(p.y-y,2)),0.5);
	}
	float x,y;
	std::uint8_t r,g,b;	
};

class Helios
{
public:
    
    Helios(int _pps=20000, int _blank_offset=0, int _intensity=255, int _ttlthreshold=0, int _device = 0,int _subdivide=DEFAULT_SUBDIVIDE,int _blank_num=DEFAULT_BLANK_NUM,int _max_angle=DEFAULT_MAX_ANGLE, int _raw_mode=DEFAULT_RAW_MODE, int _max_status_poll=DEFAULT_MAX_STATUS_POLL, int _start_immediately=0, int _single_mode=0, int _dont_block=0, int _flipx=0, int _flipy=0, float _scale=1.0, float _offset_x=0.0, float _offset_y=0.0, float _keystone_x=0.0, float _keystone_y=0.0, bool _enabled=false)
    {
        std::cout << "Helios v "<<HELIOS_VERSION;

        int numdevices=dac.OpenDevices();
        for (int i=0;i<numdevices;i++){
            std::cout << ": found laser DAC: firmware v "<<dac.GetFirmwareVersion(i)<<std::endl;
        }
        if (!numdevices){
            std::cout << ": no devices found"<<std::endl;
            device=HELIOS_NODEVICE;
        }
        else if (_device>=numdevices){
            std::cout << ": could not open device "<<_device<<std::endl;
            device=HELIOS_NODEVICE;
        }
        else {
            device=_device;
            //pps=_pps;
            dac.SetShutter(device,false);
        }
        pps=_pps;
        blank_offset = _blank_offset;
        intensity=_intensity;
        ttlthreshold = _ttlthreshold;
        subdivide=_subdivide;
        blank_num=_blank_num;
        max_angle=_max_angle;
        raw_mode=_raw_mode;
        start_immediately = _start_immediately;
        single_mode=_single_mode;
        dont_block=_dont_block;
        max_status_poll = _max_status_poll;
        flip_x = _flipx;
        flip_y = _flipy;
        offset_x = _offset_x;
        offset_y = _offset_y;
        scale = _scale;
        scale_x = 1.0;
        scale_y = 1.0;
        keystone_x = _keystone_x;
        keystone_y = _keystone_y;
        enabled=_enabled;
        output_centre=point(0x800,0x800); // 2048
    }
    
    ~Helios()
    {
        dac.CloseDevices();
    }

    void set_centre(point c){
        point cp=c+point(0x800,0x800);
        if (cp!=output_centre){
            output_centre=cp;
            //std::cout << "Helios v "<<HELIOS_VERSION<<": set output centre to "<<output_centre.x<<","<<output_centre.y<<std::endl;
        }
    }
    void set_pps(int n){
        if (n!=pps && n >= 10 && n <= 0xffff){
            pps=n;
            //std::cout << "Helios v "<<HELIOS_VERSION<<": set PPS to "<<pps<<std::endl;
        }
    }
    void set_blank_offset(int n){
        if (n!=blank_offset){
            blank_offset=n;
            //std::cout << "Helios v "<<HELIOS_VERSION<<": set PPS to "<<pps<<std::endl;
        }
    }
    void set_intensity(int i){
        if (i>-1&&i<256&&i!=intensity){
            intensity=i;
            //std::cout << "Helios v "<<HELIOS_VERSION<<": set intensity to "<<intensity<<std::endl;
        }
    }
    void set_ttlthreshold(int i){
        if (i> -1 && i <256 && i != ttlthreshold) {
            ttlthreshold=i;
            //std::cout << "Helios v "<<HELIOS_VERSION<<": set intensity to "<<intensity<<std::endl;
        }
    }
    void set_subdivide(int i){
        if (i>0&&i<256&&i!=subdivide){
            subdivide=i;
            //std::cout << "Helios v "<<HELIOS_VERSION<<": set subdivide to "<<subdivide<<std::endl;
        }
    }
    void set_blanknum(int i){
        if (i>-1&&i<256&&i!=blank_num){
            blank_num=i;
            //std::cout << "Helios v "<<HELIOS_VERSION<<": set blank_num to "<<blank_num<<std::endl;
        }
    }
    void set_rawmode(int i){
        if (i == 0 || i == 1) {
            raw_mode = i;
            std::cout << "Helios v "<<HELIOS_VERSION<<": set raw_mode to "<<raw_mode<<std::endl;
        }
    }
    void set_maxstatuspoll(int i){
        if (i > 0) {
            max_status_poll = i;
            std::cout << "Helios v "<<HELIOS_VERSION<<": set max_status_poll to "<<max_status_poll<<std::endl;
        }
    }
    void set_startimmediately(int i){
        if (i == 0 || i == 1) {
            start_immediately = i;
            std::cout << "Helios v "<<HELIOS_VERSION<<": set start_immediately to "<<start_immediately<<std::endl;
        }
    }
    void set_singlemode(int i){
        if (i == 0 || i == 1) {
            single_mode = i;
            std::cout << "Helios v "<<HELIOS_VERSION<<": set single_mode to "<<single_mode<<std::endl;
        }
    }
    void set_dontblock(int i){
        if (i == 0 || i == 1) {
            dont_block = i;
            std::cout << "Helios v "<<HELIOS_VERSION<<": set dont_block to "<<dont_block<<std::endl;
        }
    }

    void set_flip_x(int i){
        if (i == 0 || i == 1) {
            flip_x = i;
            std::cout << "Helios v "<<HELIOS_VERSION<<": set flip_x to "<<flip_x<<std::endl;
        }
    }
    void set_flip_y(int i){
        if (i == 0 || i == 1) {
            flip_y = i;
            std::cout << "Helios v "<<HELIOS_VERSION<<": set flip_y to "<<flip_y<<std::endl;
        }
    }

    void set_offset_x(float f) {
        if (f > -4096.0 && f < 4096.0) {
            offset_x = f;
        }
    }
    void set_offset_y(float f) {
        if (f > -4096.0 && f < 4096.0) {
            offset_y = f;
        }
    }

    void set_scale(float f){
        scale = f;
    }
    void set_scale_x(float f){
        scale_x = f;
    }
    void set_scale_y(float f){
        scale_y = f;
    }

    void set_keystone_x(float f){
        if (f >= -1.0 and f <= 1.0) {
            keystone_x = f;
        }
    }

    void set_keystone_y(float f){
        if (f >= -1.0 and f <= 1.0) {
            keystone_y = f;
        }
    }

    void set_maxangle(float f){
        if (f>=0.0f&&f<90.0f&&f!=max_angle){
            max_angle=f;
            //std::cout << "Helios v "<<HELIOS_VERSION<<": set max_angle to "<<max_angle<<std::endl;
        }
    }
    void set_enabled(bool e){
        if (e!=enabled){
        	enabled=e;
            //std::cout << "Helios v "<<HELIOS_VERSION<<": "<<(enabled?"enabled":"disabled")<<std::endl;
        	if (!enabled){
                if (device!=HELIOS_NODEVICE){ 
                    dac.Stop(device);
                }
        	}
        	
        }
    }

    int get_blank_offset() {
        return blank_offset;
    }

    int get_ttlthreshold() {
        return ttlthreshold;
    }

    int get_rawmode() {
        return raw_mode;
    }

    int get_pts(){
        return pps;
    }

    int get_flip_x() {
        return flip_x;
    }
    int get_flip_y() {
        return flip_y;
    }
    float get_offset_x() {
        return offset_x;
    }
    float get_offset_y() {
        return offset_y;
    }
    float get_scale() {
        return scale;
    }
    float get_scale_x() {
        return scale_x;
    }
    float get_scale_y() {
        return scale_y;
    }
    float get_keystone_x() {
        return keystone_x;
    }
    float get_keystone_y() {
        return keystone_y;
    }

    //draw a new set of points
    int draw(std::vector <point> &points);

    //draw a new set of points, without dwell, subdivision etc.
    int draw_raw();

    //repeat previous drawing (intensity is re-calculated)
    int draw();

    private:

        int device;
        HeliosDac dac;
        int pps;
        int blank_offset;
        int intensity;
        int ttlthreshold;

        bool enabled;

        int subdivide,blank_num;
        float max_angle;
        int raw_mode;
        int max_status_poll;
        int start_immediately;
        int single_mode;
        int dont_block;
        int flip_x;
        int flip_y;
        float offset_x;
        float offset_y;
        float scale; // master scale
        float scale_x;
        float scale_y;
        float keystone_x;
        float keystone_y;
        int debug;

        point output_centre;

        std::vector <point> line;
    
};

#endif //Helios_h
