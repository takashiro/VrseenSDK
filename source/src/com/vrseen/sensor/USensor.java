package com.vrseen.sensor;



import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Timer;
import java.util.TimerTask;


import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbRequest;

public class USensor {
	private UsbManager usbManager = null;
	private UsbDevice usbDevice = null;
	private UsbInterface usbInterface = null;
	private UsbEndpoint usbEndpoint = null;
	private UsbDeviceConnection usbDeviceConnection = null;

	private USensorListener uSensorListener = null;

	byte[] data;
	byte[] sensordata;

	private ByteBuffer buffer = null;
	private UsbRequest request = null;

    int count = 0;

	private Timer mTimer = null;

	private boolean mConnected;

	private boolean isPause;

	static private final class UsbDeviceInfo {
		int ventorId;
		int productId;

		UsbDeviceInfo(int ventorId, int productId) {
			this.ventorId = ventorId;
			this.productId = productId;
		}
	}
	static private final UsbDeviceInfo[] mUsbDeviceList = {
		new UsbDeviceInfo(10291, 1),
		new UsbDeviceInfo(6610, 1536)
	};

	private final long mNativePointer = NativeUSensor.ctor();

	private Context mContext;

	private static final String ACTION_USB_PERMISSION = "com.VRSeen.sensor.USB_PERMISSION";


	USensor(Context context) {
		mContext = context;
		IntentFilter filterPermission = new IntentFilter(ACTION_USB_PERMISSION);
		IntentFilter filterUsbAttach = new IntentFilter(
				UsbManager.ACTION_USB_DEVICE_ATTACHED);
		IntentFilter filterUsbDetach = new IntentFilter(
				UsbManager.ACTION_USB_DEVICE_DETACHED);

		mContext.registerReceiver(mUsbReceiver, filterPermission);
		mContext.registerReceiver(mUsbReceiver, filterUsbAttach);
		mContext.registerReceiver(mUsbReceiver, filterUsbDetach);

		usbManager = (UsbManager) mContext
				.getSystemService(Context.USB_SERVICE);
		//intent = new Intent(context, USensor.class);

		buffer = ByteBuffer.allocate(100);

		data = new byte[64];
		sensordata = new byte[64];

		mConnected = false;
		isPause = true;
	}

	public boolean Open() {
		if (GetUsbDevice()) {

			return true;

		} else {
			return false;
		}

	}

	public boolean GetUsbDevice() {

		// mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);

		if (usbManager == null) {
			return false;
		} else {
			if (enumerateDevice()) {
				if (!IsPermission()) {
					RequestPermission();
				} else {
					if (getDeviceInterface()) {
						if (getEndpoint()) {

							if (GetConnection()) {
								if (uSensorListener != null) {
									uSensorListener.onAttached();
								}

								// buffer = ByteBuffer.allocate(100);
								SetKeepAlive(10000);
								if(request == null){
									request = new UsbRequest();
								}

								request.initialize(usbDeviceConnection,
										usbEndpoint);

								//mKeepTimer = new Timer();
								//mKeepTimer.scheduleAtFixedRate(
								//		new KeepAliveTask(this), 0, 9000);
								mTimer = new Timer();
								mTimer.scheduleAtFixedRate(
										new USensorTask(this), 0, 2);

								mConnected = true;
								return true;
							}

						}
					}
				}
			}

		}
		return false;
	}

	void registerListener(USensorListener listener) {
		uSensorListener = listener;
	}

	void pause() {
		isPause = true;
	}

	void resume() {
		isPause = false;
	}

	void close() {
		if (mConnected) {
			System.out.println("close");

			//mKeepTimer.cancel();
			//mKeepTimer.purge();
			mTimer.cancel();
			mTimer.purge();


			request.cancel();
			request.close();
			usbDeviceConnection.releaseInterface(usbInterface);
			usbDeviceConnection.close();




			usbDevice = null;
			usbInterface = null;
			usbEndpoint = null;
			mConnected = false;
			usbManager =  null;

			buffer.clear() ;
			sensordata = null;
			mContext.unregisterReceiver(mUsbReceiver);
		}
	}

	private BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {

		@Override
		public void onReceive(Context context, Intent intent) {
			String actionString = intent.getAction();
			if (ACTION_USB_PERMISSION.equals(actionString)) {
				synchronized (this) {

				}
			}else if (UsbManager.ACTION_USB_DEVICE_ATTACHED
					.equals(actionString)) {
				UsbDevice device = (UsbDevice) intent
						.getParcelableExtra(UsbManager.EXTRA_DEVICE);

				UsbAttached(device);

			} else if (UsbManager.ACTION_USB_DEVICE_DETACHED
					.equals(actionString)) {
				UsbDevice device = (UsbDevice) intent
						.getParcelableExtra(UsbManager.EXTRA_DEVICE);

				UsbDetached(device);
			}
		}
	};

	static private boolean isValidProduct(int ventorId, int productId) {
		for (UsbDeviceInfo usb : mUsbDeviceList) {
			if (usb.ventorId == ventorId && usb.productId == productId) {
				return true;
			}
		}
		return false;
	}

	private boolean UsbAttached(UsbDevice device) {
		int Vid;
		int Pid;

		Vid = device.getVendorId();
		Pid = device.getProductId();
		System.out.println("usb attach" + " " + Vid + " " + Pid);

		if (isValidProduct(Vid, Pid)) {
			usbDevice = device;
			if (!IsPermission()) {
				RequestPermission();
			} else {

				if (getDeviceInterface()) {
					if (getEndpoint()) {

						if (GetConnection()) {
							if (uSensorListener != null) {
								uSensorListener.onAttached();
							}

							// buffer = ByteBuffer.allocate(100);
							if(request == null){
								request = new UsbRequest();
							}
							//request = new UsbRequest();
							request.initialize(usbDeviceConnection, usbEndpoint);

							//mKeepTimer = new Timer();
							//mKeepTimer.scheduleAtFixedRate(new KeepAliveTask(
							//		this), 0, 9000);
							mTimer = new Timer();
							mTimer.scheduleAtFixedRate(new USensorTask(this),
									0, 2);
							mConnected = true;
							System.out.println("connect true");
							return true;
						}

						return true;
					}
				}
			}
			return false;
		}
		return true;
	}

	private boolean UsbDetached(UsbDevice device) {
		int Vid = device.getVendorId();
		int Pid = device.getProductId();
		System.out.println("usb dettach" + " " + Vid + " " + Pid);

		if (isValidProduct(Vid, Pid)) {
			if (uSensorListener != null) {
				uSensorListener.onDetached();
			}
			mConnected = false;
			//mKeepTimer.cancel();
			//mKeepTimer.purge();
			if (mTimer != null) {
				mTimer.cancel();
				mTimer.purge();
			}

			uSensorListener.onSensorErrorDetected();
			System.out.println("UsbDetached");
		}

		return true;
	}

	public boolean enumerateDevice() {
		HashMap<String, UsbDevice> deviceList = usbManager.getDeviceList();
		if (!(deviceList.isEmpty())) {

			Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();
			while (deviceIterator.hasNext()) {
				UsbDevice device = deviceIterator.next();
				String strName = device.getDeviceName();
				int Vid = device.getVendorId();
				int Pid = device.getProductId();
				System.out.println("DeviceInof" + strName + "," + Vid + ","
						+ Pid);

				if (isValidProduct(Vid, Pid)) {
					usbDevice = device;
				}
			}
		}

		if (usbDevice == null) {
			return false;
		} else {
			return true;
		}
	}

	public boolean getDeviceInterface() {
		if (usbDevice != null) {
			int count = usbDevice.getInterfaceCount();
			System.out.println("device interface count = " + count);
			for (int i = 0; i < count; i++) {
				UsbInterface intf = usbDevice.getInterface(i);

				int intfclass = intf.getInterfaceClass();
				int intfsubclass = intf.getInterfaceSubclass();
				int intfprotocol = intf.getInterfaceProtocol();

				if (intfclass == 3 && intfsubclass == 0 && intfprotocol == 0) { // HID�豸�������Ϣ
					usbInterface = intf;
				}
			}

		}

		if (usbInterface == null) {
			return false;
		} else {
			return true;
		}

	}

	public boolean getEndpoint() {
		int endpointcount = usbInterface.getEndpointCount();
		for (int i = 0; i < endpointcount; i++) {
			UsbEndpoint ep = usbInterface.getEndpoint(i);

			if (ep.getType() == UsbConstants.USB_ENDPOINT_XFER_BULK) {
				if (ep.getDirection() == UsbConstants.USB_DIR_IN) {

				} else if (ep.getDirection() == UsbConstants.USB_DIR_OUT) {

				}
			}

			if (ep.getType() == UsbConstants.USB_ENDPOINT_XFER_CONTROL) {

			}

			if (ep.getType() == UsbConstants.USB_ENDPOINT_XFER_INT) {
				if (ep.getDirection() == UsbConstants.USB_DIR_IN) {
					usbEndpoint = ep;
				} else if (ep.getDirection() == UsbConstants.USB_DIR_OUT) {
					// mEpIntOut = ep;
				}
			}

		}

		if (usbEndpoint == null) {
			return false;
		} else {
			return true;
		}

	}

	private boolean IsPermission() {
		if (usbManager.hasPermission(usbDevice)) {
			System.out.println("has permission");
			return true;
		} else {
			// usbManager.requestPermission(usbDevice, pIntent);
			System.out.println("no permission");
			return false;
		}
	}

	private void RequestPermission() {

		PendingIntent pIntent = PendingIntent.getBroadcast(mContext, 0,
				new Intent(ACTION_USB_PERMISSION), 0);
		usbManager.requestPermission(usbDevice, pIntent);
		System.out.println("request permission");

	}

	public boolean GetConnection() {
		UsbDeviceConnection connection = null;
		connection = usbManager.openDevice(usbDevice);
		if (connection.claimInterface(usbInterface, true)) {
			usbDeviceConnection = connection;
			return true;
		} else {
			connection.close();
			return false;
		}

	}

	void SetKeepAlive(int interval) {
		byte[] message = new byte[5];
		message[0] = USBCMD.FEATURE_KEEP_ALIVE;
		message[1] = 0;
		message[2] = 0;
		message[3] = (byte) (interval & 0x00ff);
		message[4] = (byte) ((interval & 0xff00) >> 8);

		int reqType = 0x21 | UsbConstants.USB_DIR_OUT;
		int req = USBCMD.SET_REPORT;
		int value = 0x300 + USBCMD.FEATURE_KEEP_ALIVE;
		int len = usbDeviceConnection.controlTransfer(reqType, req, value, 0,
				message, message.length, 0);

		System.out.println("count = " + count);
		count = 0;
	}

	void dispatchData() {
		if (isPause) {
			return;
		}
		try {
			request.queue(buffer, 64);
			if (usbDeviceConnection.requestWait() == request) {

				sensordata = buffer.array();
				NativeUSensor.update(mNativePointer, sensordata);
				float[] data = NativeUSensor.getData(mNativePointer);
				uSensorListener.onSensorChanged(
						 NativeUSensor.getTimeStamp(mNativePointer), data[0],
						 data[1], data[2], data[3], data[4], data[5], data[6]);

				count++;

				if(count>4500)
				{
					SetKeepAlive(10000);
				}
			}

		} catch (Exception e) {
			// TODO: handle exception
			System.out.println("usb err");
			return;
		}
	}
}

class USBCMD {
	static final byte FEATURE_CONFIG = 2;
	static final byte FEATURE_CALIBRATE = 3;
	static final byte FEATURE_RANGE = 4;
	static final byte FEATURE_REGISTER = 5;
	static final byte FEATURE_DFU = 6;
	static final byte FEATURE_GPIO = 7;
	static final byte FEATURE_KEEP_ALIVE = 8;
	static final byte FEATURE_DISPLAY_INFO = 9;
	static final byte FEATURE_SERIAL = 10;

	static final int GET_REPORT = 1;
	static final int SET_REPORT = 9;
}

class USensorTask extends TimerTask {
	private USensor mSensor = null;

	public USensorTask(USensor sensor) {
		mSensor = sensor;
	}

	@Override
	public void run() {
		mSensor.dispatchData();
	}
}


class NativeUSensor {
	static native long ctor();

	static native boolean update(long ptr, byte[] data);

	static native long getTimeStamp(long ptr);

	static native float[] getData(long ptr);
}
