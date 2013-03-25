
#ifndef DMX_USB_PRO
#define DMX_USB_PRO

#pragma once

#include "cinder/Thread.h"
#include "cinder/Serial.h"
#include "cinder/app/App.h"


#define DMXPRO_START_MSG		0x7E		// Start of message delimiter
#define DMXPRO_END_MSG			0xE7		// End of message delimiter
#define DMXPRO_SEND_LABEL		6			// Output Only Send DMX Packet Request
#define DMXPRO_BAUD_RATE		57600		// virtual COM doesn't control the usb, this is just a dummy value
#define DMXPRO_FRAME_RATE		35			// dmx send frame rate
#define DMXPRO_DATA_SIZE       	513         // include first byte 0x00, what's that?
#define DMXPRO_PACKET_SIZE      518         // data + 4 bytes(DMXPRO_START_MSG, DMXPRO_SEND_LABEL, DATA_SIZE_LSB, DATA_SIZE_MSB) at the beginning and 1 byte(DMXPRO_END_MSG) at the end

//////////////////////////////////////////////////////////
// LAST 4 dmx channels seem not to be working, 508-511 !!!
//////////////////////////////////////////////////////////

class DMXPro{

public:
    
    DMXPro( const std::string &serialDevicePath );

	~DMXPro();

	void init(bool initWithZeros = true);


	void	setZeros();
	
	bool	isConnected() { return mSerial != NULL; }
	
	void	setValue(int value, int channel);
	
	void	reconnect();
	
	void	shutdown(bool send_zeros = true);
	
	
private:
	
	void initDMX();
    
	void initSerial(bool initWithZeros);

	void			sendDMXData();

private:
    
	unsigned char	*mDMXPacket;			// DMX packet, it contains bytes
	ci::Serial		*mSerial;				// serial interface
	int				mThreadSleepFor;		// sleep for N ms, this is based on the FRAME_RATE
    std::shared_ptr<std::thread> mThread;    //this is the thread that loops
	std::mutex	mDMXDataMutex;			// mutex unique lock
	std::string		mSerialDevicePath;		// usb serial device path
    bool            mNeedsSending;           //mark this as true for sending the data out
    
    
private:
    // disallow
    DMXPro(const DMXPro&);
    DMXPro& operator=(const DMXPro&);
    
};


#endif