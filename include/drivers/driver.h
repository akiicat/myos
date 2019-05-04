#ifndef __MYOS__DRIVERS__DRIVER_H
#define __MYOS__DRIVERS__DRIVER_H

namespace myos {

  namespace drivers {

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

  }

}

#endif