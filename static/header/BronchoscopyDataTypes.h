#ifndef BRONCHOSCOPY_DATA_TYPES_H
#define BRONCHOSCOPY_DATA_TYPES_H

namespace BronchoscopyLib {

struct CameraPose {
    double position[3]{0.0, 0.0, 0.0};
    double forward[3]{0.0, 0.0, 1.0};
    double up[3]{0.0, 1.0, 0.0};
    double right[3]{1.0, 0.0, 0.0};
    double rotationMatrix[9]{
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    };
    // Euler angles in degrees, XYZ order (roll, pitch, yaw)
    double euler[3]{0.0, 0.0, 0.0};
};

struct MaterialParameters {
    double brightness = 1.0;
    double reflectivity = 0.2;
    double ambient = 0.2;
    double attenuation = 0.2;
};

} // namespace BronchoscopyLib

#endif // BRONCHOSCOPY_DATA_TYPES_H
