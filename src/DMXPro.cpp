 

#include "cinder/app/AppBasic.h"
#include "cinder/Utilities.h"
#include <iostream>
#include "DMXPro.h"

using namespace ci;
using namespace ci::app;
using namespace std;


DMXPro::DMXPro( const string &serialDevicePath ) : mSerialDevicePath(serialDevicePath), mSerial(NULL)
{	
	mThreadSleepFor = 1000 / DMXPRO_FRAME_RATE;
    
	init();
	setZeros();
	console() << "starting DMX" << std::endl;
}


DMXPro::~DMXPro()
{
	shutdown(true);
    
    //wait for the thread
    if(mThread && mThread->joinable()){
        mThread->join();
    }
    
}

void DMXPro::shutdown(bool send_zeros)
{

	if(send_zeros){
		//we set the zeroes
		setZeros();
		//wait a little bit so that we are sure it is sent
		ci::sleep( mThreadSleepFor*2 );
	}

	{
		//lock it so we can get rid of the serial
		std::lock_guard<std::mutex> dataLock(mDMXDataMutex);
		if (mSerial){
			mSerial->flush();
			mSerial = std::shared_ptr<ci::Serial>();
		}
	}
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
		mSerial = std::shared_ptr<ci::Serial>();
		ci::sleep(50);	
	}
	
	try 
    {
		Serial::Device dev = Serial::findDeviceByNameContains(mSerialDevicePath);
		mSerial = std::shared_ptr<ci::Serial>(new ci::Serial(dev, DMXPRO_BAUD_RATE));
        console() << "DMXPro > Connected to usb DMX interface: " << dev.getName() << endl;
	}
	catch( ... ) 
    {
        console() << "DMXPro > There was an error initializing the usb DMX device" << endl;
		mSerial = NULL;
	}
    
    // start thread to send data at the specific DMX_FRAME_RATE
	if(mSerial){
		mThread = std::shared_ptr<std::thread>( new std::thread(&DMXPro::sendDMXData, this) );
	}
}


void DMXPro::initDMX()
{
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
	while(true) 
    {
		{
		std::lock_guard<std::mutex> dataLock(mDMXDataMutex);			// get DMX packet UNIQUE lock
			if(!mSerial) break;

			if(mNeedsSending){
				mSerial->writeBytes(mDMXPacket, DMXPRO_PACKET_SIZE);                // send data
				mNeedsSending = false;
			}
		}
   		ci::sleep( mThreadSleepFor );
	}
}


void DMXPro::setValue(int value, int channel) 
{    
	if ( channel < 1 || channel > DMXPRO_PACKET_SIZE-2 )
	{
        console() << "DMXPro > invalid DMX channel: " << channel << endl;
        return;
	}
    // DMX channels start from byte [5] and end at byte [DMXPRO_PACKET_SIZE-2], last byte is EOT(0xE7)
    //but we start at the 1st channel, so 4+channel
	value = math<int>::clamp(value, 0, 255);
	{
	std::lock_guard<std::mutex> dataLock(mDMXDataMutex);			// get DMX packet UNIQUE lock
		mDMXPacket[ 4 + channel ] = value;                                  // update value
		mNeedsSending = true;                                                    //mark the data as changed
	}
}


void DMXPro::setZeros()
{
	std::lock_guard<std::mutex> dataLock(mDMXDataMutex);
    for (int i=5; i < DMXPRO_PACKET_SIZE-2; i++)                        // DMX channels start form byte [5] and end at byte [DMXPRO_PACKET_SIZE-2], last byte is EOT(0xE7)
		mDMXPacket[i] = 0;
    mNeedsSending = true;
}

