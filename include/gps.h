#ifndef GPS_H
#define GPS_H

// ═══════════════════════════════════════════════════════════════════════════
// HaleHound-CYD GPS Module Interface
// Hardware: NEO-6M GPS Module via Software Serial
// Created: 2026-02-06
// ═══════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "cyd_config.h"

// ═══════════════════════════════════════════════════════════════════════════
// GPS STATUS CODES
// ═══════════════════════════════════════════════════════════════════════════

enum GPSStatus {
    GPS_NO_MODULE,      // GPS not detected / not responding
    GPS_SEARCHING,      // GPS active but no fix yet
    GPS_FIX_2D,         // 2D fix (lat/lng only, no altitude)
    GPS_FIX_3D          // 3D fix (full position with altitude)
};

// ═══════════════════════════════════════════════════════════════════════════
// GPS DATA STRUCTURE
// ═══════════════════════════════════════════════════════════════════════════

struct GPSData {
    bool valid;             // True if we have a valid fix
    double latitude;        // Decimal degrees (-90 to +90)
    double longitude;       // Decimal degrees (-180 to +180)
    double altitude;        // Meters above sea level
    double speed;           // Speed in km/h
    double course;          // Course over ground in degrees
    uint8_t satellites;     // Number of satellites in view
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint32_t age;           // Age of fix in milliseconds
};

// ═══════════════════════════════════════════════════════════════════════════
// EXTERNAL OBJECTS
// ═══════════════════════════════════════════════════════════════════════════

extern TinyGPSPlus gps;
extern HardwareSerial gpsSerial;
extern GPSData gpsData;

// ═══════════════════════════════════════════════════════════════════════════
// CORE FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

// Initialize GPS module - call in setup()
void gpsSetup();

// Process GPS data - call frequently in loop()
void gpsLoop();

// Check if GPS has valid fix
bool gpsHasFix();

// Get current GPS status
GPSStatus gpsGetStatus();

// ═══════════════════════════════════════════════════════════════════════════
// LOCATION FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

// Get latitude in decimal degrees
double gpsGetLat();

// Get longitude in decimal degrees
double gpsGetLng();

// Get altitude in meters
double gpsGetAltitude();

// Get speed in km/h
double gpsGetSpeed();

// Get course in degrees (0-360)
double gpsGetCourse();

// Get number of satellites
uint8_t gpsGetSatellites();

// ═══════════════════════════════════════════════════════════════════════════
// TIME FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

// Get UTC timestamp as string: "YYYY-MM-DD HH:MM:SS"
String gpsGetTimestamp();

// Get date as string: "YYYY-MM-DD"
String gpsGetDate();

// Get time as string: "HH:MM:SS"
String gpsGetTime();

// Get Unix timestamp (seconds since 1970)
uint32_t gpsGetUnixTime();

// ═══════════════════════════════════════════════════════════════════════════
// FORMATTING FUNCTIONS (for logging/display)
// ═══════════════════════════════════════════════════════════════════════════

// Get coordinates as string: "LAT,LNG" (for CSV logging)
String gpsGetCoordsCSV();

// Get coordinates as string: "LAT, LNG" (for display)
String gpsGetCoordsDisplay();

// Get full location string: "LAT,LNG,ALT,SPD,SAT,TIMESTAMP"
String gpsGetFullLogEntry();

// Get Google Maps URL for current position
String gpsGetMapsURL();

// ═══════════════════════════════════════════════════════════════════════════
// DISTANCE/BEARING FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

// Calculate distance to a point in meters
double gpsDistanceTo(double lat, double lng);

// Calculate bearing to a point in degrees
double gpsBearingTo(double lat, double lng);

// ═══════════════════════════════════════════════════════════════════════════
// DEBUG FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

// Print GPS status to Serial
void gpsPrintStatus();

// Get status as string
String gpsGetStatusString();

#endif // GPS_H
