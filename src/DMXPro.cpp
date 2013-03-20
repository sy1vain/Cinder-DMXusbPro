 

#include "cinder/app/AppBasic.h"
#include "cinder/Utilities.h"
#include <iostream>
#include "DMXPro.h"

using namespace ci;
using namespace ci::app;
using namespace std;


DMXPro::DMXPro( const string &serialDevicePath ) : mDMXPacket(NULL), mSerialDevicePath(serialDevicePath), mSerial(NULL)
{	
	mThreadSleepFor = 1000 / DMXPRO_FRAME_RATE;
    
	init();
	
	setZeros();
}


DMXPro::~DMXPro()
{
    setZeros();
    
    ci::sleep(50);	
    
    if ( mSerial )
    {
        mSerial->flush();
        delete mSerial;
        mSerial = NULL;
    }
    
    delete []mDMXPacket;
    
    console() << "shutdown DMXPro" << endl;
}

void DMXPro::shutdown(bool send_zeros)
{
	if ( mSerial )
	{
		if (send_zeros)
			setZeros();					// send zeros to all channels
		
		ci::sleep( mThreadSleepFor*2 );
		mSerial->flush();	
		delete mSerial;
		mSerial = NULL;
		ci::sleep(50);	
	}
    console() << "DMXPro > shutdown!" << endl;
}


void DMXPro::init(bool initWithZeros) 
{
    console() << "DMXPro > Initializing device" << endl;

	initDMX();
	initSerial(initWithZeros);
}


void DMXPro::initSerial(bool initWithZeros)
{
	if ( mSerial )
	{
		if (initWithZeros)
		{
			setZeros();					// send zeros to all channels
            console() << "DMXPro > Init serial with zeros() before disconnect" << endl;
			ci::sleep(100);	
		}
		mSerial->flush();	
		delete mSerial;
		mSerial = NULL;
		ci::sleep(50);	
	}
	
	try 
    {
		Serial::Device dev = findDeviceByPathContains(mSerialDevicePath);
		mSerial = new Serial(dev, DMXPRO_BAUD_RATE);
        console() << "DMXPro > Connected to usb DMX interface: " << dev.getName() << endl;
	}
	catch( ... ) 
    {
        console() << "DMXPro > There was an error initializing the usb DMX device" << endl;
		mSerial = NULL;
	}
	thread sendDMXDataThread( &DMXPro::sendDMXData, this);				// start thread to send data at the specific DMX_FRAME_RATE 
}


void DMXPro::initDMX()
{
	delete []mDMXPacket;
	mDMXPacket	= NULL;
	mDMXPacket	= new unsigned char[DMXPRO_PACKET_SIZE];
    
    // LAST 4 dmx channels seem not to be working, 508-511 !!!
    
    for (int i=0; i < DMXPRO_PACKET_SIZE; i++)                      // initialize all channels with zeros, data starts from [5]
		mDMXPacket[i] = 0;
    
    mDMXPacket[0] = DMXPRO_START_MSG;								// DMX start delimiter 0x7E
	mDMXPacket[1] = DMXPRO_SEND_LABEL;								// set message type
    mDMXPacket[2] = (int)DMXPRO_DATA_SIZE & 0xFF;					// Data Length LSB
    mDMXPacket[3] = ((int)DMXPRO_DATA_SIZE >> 8) & 0xFF;            // Data Length MSB
	mDMXPacket[4] = 0;                                              // NO IDEA what this is for!
	mDMXPacket[DMXPRO_PACKET_SIZE-1] = DMXPRO_END_MSG;              // DMX start delimiter 0xE7
}


void DMXPro::sendDMXData() 
{
	while(mSerial) 
    {
		std::unique_lock<std::mutex> dataLock(mDMXDataMutex);			// get DMX packet UNIQUE lock
		mSerial->writeBytes(mDMXPacket, DMXPRO_PACKET_SIZE);                // send data        
		dataLock.unlock();													// unlock data
		ci::sleep( mThreadSleepFor );
	}
    console() << "DMXPro > sendDMXData() thread exited!" << endl;
}


Serial::Device DMXPro::findDeviceByPathContains( const string &searchString) 
{
	const std::vector<Serial::Device> &devices = Serial::getDevices();
	for( std::vector<Serial::Device>::const_iterator deviceIt = devices.begin(); deviceIt != devices.end(); ++deviceIt ) {
		if( deviceIt->getPath().find( searchString ) != std::string::npos )
			return *deviceIt;
	}
	return Serial::Device();
}


void DMXPro::setValue(int value, int channel) 
{    
	if ( channel < 0 || channel > DMXPRO_PACKET_SIZE-2 )
	{
        console() << "DMXPro > invalid DMX channel: " << channel << endl;
        return;
	}
    // DMX channels start form byte [5] and end at byte [DMXPRO_PACKET_SIZE-2], last byte is EOT(0xE7)        
	value = math<int>::clamp(value, 0, 255);
	std::unique_lock<std::mutex> dataLock(mDMXDataMutex);			// get DMX packet UNIQUE lock
	mDMXPacket[ 5 + channel ] = value;                                  // update value
	dataLock.unlock();													// unlock mutex
}


void DMXPro::setZeros()
{
    for (int i=5; i < DMXPRO_PACKET_SIZE-2; i++)                        // DMX channels start form byte [5] and end at byte [DMXPRO_PACKET_SIZE-2], last byte is EOT(0xE7)
		mDMXPacket[i] = 0;
}

