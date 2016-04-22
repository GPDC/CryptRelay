#ifndef port_knock_h__
#define port_knock_h__

class PortKnock
{
public:
	PortKnock();
	~PortKnock();

	// Very simple checking of 1 port. Not for checking many ports quickly.
	bool isPortOpen(std::string ip, std::string port);

protected:
private:
};

#endif//port_knock_h__