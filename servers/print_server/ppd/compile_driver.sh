#! /bin/bash

# This script is used to compile a PPD file for a printer driver.
# Input is the driver.drv file
# For reference on how to develop a driver, see the driver.drv file

ppdc driver.drv -d ./ 