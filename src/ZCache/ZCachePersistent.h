#pragma once
class ZCachePersistent
{
public:
	ZCachePersistent();
	~ZCachePersistent();
protected:
    FILE * m_saveFile;
private:
};