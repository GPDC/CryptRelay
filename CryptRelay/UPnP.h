#ifndef UPnP_h__
#define UPnP_h__
#include <UPnP.h>

class UPnP : public IUPnPDeviceFinderCallback
{
public:
	UPnP();
	~UPnP();

	bool startUPnP();

#ifdef _WIN32
	HRESULT findUPnPDevice(BSTR TypeURI);



	HRESULT STDMETHODCALLTYPE DeviceAdded(
		/* [in] */ LONG lFindData,
		/* [in] */ __RPC__in_opt IUPnPDevice *pDevice);

	HRESULT STDMETHODCALLTYPE DeviceRemoved(
		/* [in] */ LONG lFindData,
		/* [in] */ __RPC__in BSTR bstrUDN);

	HRESULT STDMETHODCALLTYPE SearchComplete(
		/* [in] */ LONG lFindData);



	HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);

	ULONG STDMETHODCALLTYPE AddRef(void);

	ULONG STDMETHODCALLTYPE Release(void);

#endif// _WIN32

protected:
private:

#ifdef _WIN32
	LONG m_lRefCount;
#endif// _WIN32
};

#endif