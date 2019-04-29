#ifndef __DRIVER_H
#define __DRIVER_H

class Driver {
  public:
    Driver();
    ~Driver();

    virtual void Activate();
    int Reset();
    void Deactivate();
};

class DriverManager {
  private:
    Driver* drivers[255];
    int numDrivers;

  public:
    DriverManager();
    void AddDriver(Driver*);
    void ActivateAll();
};

#endif
