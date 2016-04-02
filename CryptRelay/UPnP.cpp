//UPnP.cpp
#include <stdio.h>
#include <iostream>	//cout
#include "UPnP.h"

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "user32.lib")

UPnP::UPnP()
{
#ifdef _WIN32
	m_lRefCount = 0;// what is this
#endif// _WIN32
}
UPnP::~UPnP()
{

}

bool UPnP::startUPnP()
{
#ifdef _WIN32

	

#endif//WIN32

#ifdef __linux__



#endif//__linux__


	return true;
}





#ifdef _WIN32

HRESULT UPnP::findUPnPDevice(BSTR TypeURI)
{
	HRESULT hr = S_OK;

	IUPnPDeviceFinderCallback* pUPnPDeviceFinderCallback = new UPnP();
	if (pUPnPDeviceFinderCallback != NULL)
	{
		pUPnPDeviceFinderCallback->AddRef();
		IUPnPDeviceFinder* pUPnPDeviceFinder;
		hr = CoCreateInstance(
			CLSID_UPnPDeviceFinder,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IUPnPDeviceFinder,
			reinterpret_cast<void**>(&pUPnPDeviceFinder)
		);
		if (SUCCEEDED(hr))
		{
			LONG lFindData;
			hr = pUPnPDeviceFinder->CreateAsyncFind(
				TypeURI,
				0,
				pUPnPDeviceFinderCallback,
				&lFindData
			);
			if (SUCCEEDED(hr))
			{
				hr = pUPnPDeviceFinder->StartAsyncFind(lFindData);
				if (SUCCEEDED(hr))
				{
					// STA threads must pump messages ???
					MSG Message;
					BOOL bGetMessage;
					while (bGetMessage = GetMessage(&Message, NULL, 0, 0)
						&& -1 != bGetMessage)
					{
						DispatchMessage(&Message);
					}
				}
				pUPnPDeviceFinder->CancelAsyncFind(lFindData);
			}
			pUPnPDeviceFinder->Release();
		}
		pUPnPDeviceFinderCallback->Release();
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}
	return hr;
}


HRESULT STDMETHODCALLTYPE UPnP::DeviceAdded(LONG lFindData, __RPC__in_opt IUPnPDevice *pDevice)
{
	HRESULT hr = S_OK;

	// save pDevice so it can be accessed outside scope of this method???

	BSTR UniqueDeviceName;
	hr = pDevice->get_UniqueDeviceName(&UniqueDeviceName);
	if (SUCCEEDED(hr))
	{
		BSTR FriendlyName;
		hr = pDevice->get_FriendlyName(&FriendlyName);
		if (SUCCEEDED(hr))
		{
			std::cout << "Device Added: udn: " << FriendlyName << ", name: " << UniqueDeviceName << "\n";
		}
		::SysFreeString(UniqueDeviceName);
	}

	return hr;
}
HRESULT STDMETHODCALLTYPE UPnP::DeviceRemoved(LONG lFindData, __RPC__in BSTR bstrUDN)
{
	std::cout << "Device Removed: udn: " << bstrUDN << "\n";
	return S_OK;
}
HRESULT STDMETHODCALLTYPE UPnP::SearchComplete(LONG lFindData)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UPnP::QueryInterface(REFIID iid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	HRESULT hr = S_OK;

	if (ppvObject == NULL)
	{
		hr = E_POINTER;
	}
	else
	{
		*ppvObject = NULL;
	}

	if (SUCCEEDED(hr))
	{
		if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IUPnPDeviceFinderCallback))
		{
			*ppvObject = static_cast<IUPnPDeviceFinderCallback*>(this);
			AddRef();
		}
		else
		{
			hr = E_NOINTERFACE;
		}
	}
	return hr;
}

ULONG STDMETHODCALLTYPE UPnP::AddRef(void)
{
	return ::InterlockedIncrement(&m_lRefCount);
}

ULONG STDMETHODCALLTYPE UPnP::Release(void)
{
	LONG lRefCount = ::InterlockedDecrement(&m_lRefCount);
	if (0 == lRefCount)
	{
		delete this;
	}

	return lRefCount;
}

#endif// _WIN32