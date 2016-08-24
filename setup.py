from distutils.core import setup, Extension

module1 = Extension('modbus_client',
                    include_dirs = ['libmodbus'],
                    sources = ['modbus_client.c',
                               'libmodbus/modbus.c',
                               'libmodbus/modbus-rtu.c',
                               'libmodbus/modbus-data.c'])

setup (name = 'Modbus Client',
       version = '1.0',
       description = 'Communicate with MODBUS clients vis libmodbus',
       ext_modules = [module1])
