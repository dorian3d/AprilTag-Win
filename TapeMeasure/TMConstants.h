//
//  Constants.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/29/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#ifndef TapeMeasure_Constants_h
#define TapeMeasure_Constants_h
#endif

#define ENTITY_MEASUREMENT @"TMMeasurement"
#define ENTITY_LOCATION @"TMLocation"
#define DATA_MODEL_URL @"TMMeasurementDM"

#define CAPTURE_DATA YES

#define DATA_MANAGER [TMDataManagerFactory getDataManagerInstance]
#define SESSION_MANAGER [TMAvSessionManagerFactory getAVSessionManagerInstance]
#define CORVIS_MANAGER [TMCorvisManagerFactory getCorvisManagerInstance]
#define VIDEOCAP_MANAGER [TMVideoCapManagerFactory getVideoCapManagerInstance]
#define MOTIONCAP_MANAGER [TMMotionCapManagerFactory getMotionCapManagerInstance]
#define LOCATION_MANAGER [TMLocationManagerFactory getLocationManagerInstance]

#define DOCUMENTS_DIRECTORY [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject]

//these are assigned values in TMDistanceFormatter.m
extern const float METERS_PER_INCH;
extern const float INCHES_PER_METER;
extern const int INCHES_PER_FOOT;
extern const int INCHES_PER_YARD;
extern const int INCHES_PER_MILE;

typedef enum {
    TypePointToPoint = 0, TypeTotalPath = 1, TypeHorizontal = 2, TypeVertical = 3
} MeasurementType;