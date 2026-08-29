/**
 * Camera3D.h
 *
 * Minimal hand-rolled camera math for the Camera Path Editor's orbit viewport
 * (Maya-style spherical orbit around a pivot). No glm - this codebase hand-rolls
 * its own math throughout, and the actual need here is just lookAt + perspective
 * + point transform.
 *
 * License: GPL3
 */

#ifndef CAMERA3D_H
#define CAMERA3D_H

#include <cmath>
#include <algorithm>

// Column-major 4x4 matrix, GL convention (m[col*4+row]) - ready to hand to
// glUniformMatrix4fv with transpose=GL_FALSE.
struct Mat4 {
    float m[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
};

inline Mat4 mat4Identity() {
    return Mat4();
}

// c = a * b (applies b first, then a - i.e. c*v == a*(b*v))
inline Mat4 mat4Multiply(const Mat4& a, const Mat4& b) {
    Mat4 c;
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += a.m[k * 4 + row] * b.m[col * 4 + k];
            }
            c.m[col * 4 + row] = sum;
        }
    }
    return c;
}

// out (vec4, homogeneous) = m * in (treated as a point, w=1)
inline void mat4TransformPoint(const Mat4& m, const float in[3], float out[4]) {
    for (int row = 0; row < 4; row++) {
        out[row] = m.m[0 * 4 + row] * in[0] +
                   m.m[1 * 4 + row] * in[1] +
                   m.m[2 * 4 + row] * in[2] +
                   m.m[3 * 4 + row] * 1.0f;
    }
}

inline Mat4 mat4LookAt(const float eye[3], const float target[3], const float up[3]) {
    float fx = target[0] - eye[0], fy = target[1] - eye[1], fz = target[2] - eye[2];
    float flen = std::sqrt(fx*fx + fy*fy + fz*fz);
    if (flen < 1e-6f) flen = 1e-6f;
    fx /= flen; fy /= flen; fz /= flen;

    // right = normalize(forward x up)
    float rx = fy*up[2] - fz*up[1];
    float ry = fz*up[0] - fx*up[2];
    float rz = fx*up[1] - fy*up[0];
    float rlen = std::sqrt(rx*rx + ry*ry + rz*rz);
    if (rlen < 1e-6f) rlen = 1e-6f;
    rx /= rlen; ry /= rlen; rz /= rlen;

    // realUp = right x forward
    float ux = ry*fz - rz*fy;
    float uy = rz*fx - rx*fz;
    float uz = rx*fy - ry*fx;

    Mat4 view;
    view.m[0] = rx;  view.m[4] = ry;  view.m[8]  = rz;  view.m[12] = -(rx*eye[0] + ry*eye[1] + rz*eye[2]);
    view.m[1] = ux;  view.m[5] = uy;  view.m[9]  = uz;  view.m[13] = -(ux*eye[0] + uy*eye[1] + uz*eye[2]);
    view.m[2] = -fx; view.m[6] = -fy; view.m[10] = -fz; view.m[14] =  (fx*eye[0] + fy*eye[1] + fz*eye[2]);
    view.m[3] = 0;   view.m[7] = 0;   view.m[11] = 0;   view.m[15] = 1;
    return view;
}

// fovYRadians: full vertical field of view. Standard GL perspective (right-handed,
// maps view-space Z in [-near,-far] to clip-space Z in [-1,1]).
inline Mat4 mat4Perspective(float fovYRadians, float aspect, float nearZ, float farZ) {
    float f = 1.0f / std::tan(fovYRadians * 0.5f);
    Mat4 proj;
    for (int i = 0; i < 16; i++) proj.m[i] = 0.0f;
    proj.m[0] = f / aspect;
    proj.m[5] = f;
    proj.m[10] = (farZ + nearZ) / (nearZ - farZ);
    proj.m[11] = -1.0f;
    proj.m[14] = (2.0f * farZ * nearZ) / (nearZ - farZ);
    return proj;
}

// Spherical orbit camera: azimuth/elevation/distance around a pivot point, matching
// CrossViewWarp's own azimuth/elevation/distance/hfov convention exactly - hfovDeg is
// shared 1:1 with GenerationParams::loraCameraHfovN so what the user sees while orbiting
// is the same camera the LoRA will actually render with.
struct OrbitCamera {
    float azimuthDeg = 0.0f;
    float elevationDeg = 0.0f;
    // Checked directly against crossview_warp_node.py: CrossViewWarp's own "distance" is
    // MULTIPLICATIVE relative to the source camera (eye = pivot + distance*(sourceCameraPos -
    // pivot)), not an absolute world-space radius - 1.0 means "unchanged from the source", with a
    // node-enforced range of [0.1, 3.0]. This value is sent straight through as that literal
    // parameter, so it has to live in the same units/range, not this editor's own preview-scale
    // orbit radius (which is what it used to be, defaulting to an arbitrary 0.85 with no range
    // tie to the node at all).
    float distance = 1.0f;
    float hfovDeg = 50.0f;
    // Literally CrossViewWarp's pivot_x/y/z with pivot_override=true - real METRIC metres in the
    // SERVER's own frame (OpenCV-style: source camera at the origin, +Z forward/away from camera,
    // +Y DOWN - matches _warp_frame's own unprojection [(u-cx)/fx*z, (v-cy)/fx*z, z]). The point
    // cloud (buildPointCloudsThreadFunc()) and this camera's own eye/target math both stay in
    // this exact frame with no flip - see getViewMatrix()'s comment for why a Y-flip (an earlier
    // version of this code) mirrors the render on X instead of just reorienting it. Pan (Alt+MMB,
    // handleOrbitInput()) moves pivot[0]/[1] directly (not view-relative - see that comment for
    // why a basis-derived pan direction was actually broken); Ctrl+scroll moves pivot[2].
    float pivot[3] = {0.0f, 0.0f, 1.05f};

    static constexpr float kMaxElevationDeg = 89.0f;  // numerical gimbal-flip guard only -
                                                        // NOT the LoRA safe-zone band (that's
                                                        // soft-warning-only UI, see the editor's
                                                        // safe-zone HUD)

    void clampElevation() {
        if (elevationDeg > kMaxElevationDeg) elevationDeg = kMaxElevationDeg;
        if (elevationDeg < -kMaxElevationDeg) elevationDeg = -kMaxElevationDeg;
    }

    // azimuthDeg accumulates unbounded during a drag (nothing stops the user from spinning past
    // +-180 any number of times, especially now that orbit dragging has no edge-of-screen limit).
    // The eye position itself wraps around fine either way (sin/cos are periodic), but
    // CrossViewWarp validates the literal azimuth value it's given to [-180,180] and rejects
    // anything outside that - wrap back into range after every update so what actually gets
    // stored/applied always matches what the LoRA will accept.
    void wrapAzimuth() {
        azimuthDeg = std::fmod(azimuthDeg + 180.0f, 360.0f);
        if (azimuthDeg < 0.0f) azimuthDeg += 360.0f;
        azimuthDeg -= 180.0f;
    }

    // Bit-for-bit port of crossview_warp_node.py's _orbit_C_tgt(az_deg, el_deg, dist, pivot,
    // aim=None) - eye = pivot + dist*(Ry(-az) @ Rx(-el) @ (-pivot)), target = pivot (matches
    // keep_source_aim=false, this editor's/generation's own default). Both eye and target come
    // out in the SERVER's Y-down frame - see `pivot`'s own comment above.
    void getEyeAndTargetServerFrame(float outEye[3], float outTarget[3]) const {
        float elRad = (-elevationDeg) * (float)M_PI / 180.0f;
        float azRad = (-azimuthDeg) * (float)M_PI / 180.0f;
        float w[3] = {-pivot[0], -pivot[1], -pivot[2]};
        // Rx(elRad) @ w
        float c1 = std::cos(elRad), s1 = std::sin(elRad);
        float w1[3] = {w[0], c1 * w[1] - s1 * w[2], s1 * w[1] + c1 * w[2]};
        // Ry(azRad) @ w1
        float c2 = std::cos(azRad), s2 = std::sin(azRad);
        float w2[3] = {c2 * w1[0] + s2 * w1[2], w1[1], -s2 * w1[0] + c2 * w1[2]};
        for (int k = 0; k < 3; k++) {
            outEye[k] = pivot[k] + distance * w2[k];
            outTarget[k] = pivot[k];
        }
    }

    // Solves for the (azimuthDeg, elevationDeg) that make getEyeAndTargetServerFrame() look in
    // exactly `desiredForward` when the pivot is `newPivot` (distance held fixed). Used by pan
    // (Alt+MMB, handleOrbitInput()) to auto-compensate az/el after moving the pivot: since
    // target=pivot always (keep_source_aim=false, matching the real generation), and
    // forward = target-eye = dist*(Ry(-az)@Rx(-el)@pivot) (direction-only, dist cancels on
    // normalize), moving the pivot alone changes forward - i.e. re-aims the camera - unless az/el
    // are adjusted to compensate. That "re-aims instead of panning" was reported directly:
    // "alt+middlemouse works totally wrong changes view direction".
    //
    // Derivation (matches getEyeAndTargetServerFrame()'s own Rx-then-Ry order/signs exactly):
    // let v = normalize(newPivot), w = desiredForward (already unit length), a = azRad, b = elRad
    // (so azimuthDeg = -a*180/pi, elevationDeg = -b*180/pi, as that function computes them).
    // forward = Ry(a) @ Rx(b) @ v = w.
    //   Rx(b) preserves the x-component and only mixes y/z: u = Rx(b)@v, u.x = v.x.
    //   Ry(a) preserves the y-component and only mixes x/z, so its OUTPUT y-component is u.y
    //   unchanged - meaning w.y = u.y = cos(b)*v.y - sin(b)*v.z is the one equation solvable for b
    //   alone (a doesn't appear in it at all). Standard A*cos(b)+B*sin(b)=C form with A=v.y,
    //   B=-v.z, C=w.y: b = atan2(B,A) +/- acos(C/sqrt(A^2+B^2)) - two solutions (an elevation
    //   ambiguity, same shape as solving asin), resolved by picking whichever is closer to
    //   `elevationHintDeg` (the pre-pan elevation) so a smooth drag doesn't jump branches.
    //   With b fixed, u is fully known; solving Ry(a)@u=w for the remaining (x,z) rotation gives
    //   a = atan2(u.z,u.x) - atan2(w.z,w.x) (verified by direct substitution).
    // Returns false (leaving az/el untouched) only if newPivot or desiredForward is degenerate
    // (near-zero length) or no real solution exists even at the clamped closest approach.
    bool solveAzElForForward(const float newPivot[3], const float desiredForward[3],
                              float azimuthHintDeg, float elevationHintDeg,
                              float& outAzimuthDeg, float& outElevationDeg) const {
        float vlen = std::sqrt(newPivot[0]*newPivot[0] + newPivot[1]*newPivot[1] + newPivot[2]*newPivot[2]);
        if (vlen < 1e-6f) return false;
        float v[3] = {newPivot[0]/vlen, newPivot[1]/vlen, newPivot[2]/vlen};
        float wlen = std::sqrt(desiredForward[0]*desiredForward[0] + desiredForward[1]*desiredForward[1] +
                                desiredForward[2]*desiredForward[2]);
        if (wlen < 1e-6f) return false;
        float w[3] = {desiredForward[0]/wlen, desiredForward[1]/wlen, desiredForward[2]/wlen};

        float A = v[1], B = -v[2], C = w[1];
        float amp = std::sqrt(A*A + B*B);
        if (amp < 1e-6f) return false;
        float Cn = std::max(-1.0f, std::min(1.0f, C / amp));
        float phi = std::atan2(B, A);
        float acosC = std::acos(Cn);
        float bCandidates[2] = {phi + acosC, phi - acosC};

        float elHintRad = (-elevationHintDeg) * (float)M_PI / 180.0f;
        float b = bCandidates[0];
        float bestDiff = std::fabs(std::fmod(bCandidates[0] - elHintRad + (float)M_PI, 2.0f * (float)M_PI) - (float)M_PI);
        for (int k = 1; k < 2; k++) {
            float diff = std::fabs(std::fmod(bCandidates[k] - elHintRad + (float)M_PI, 2.0f * (float)M_PI) - (float)M_PI);
            if (diff < bestDiff) { bestDiff = diff; b = bCandidates[k]; }
        }

        float cb = std::cos(b), sb = std::sin(b);
        float u[3] = {v[0], cb*v[1] - sb*v[2], sb*v[1] + cb*v[2]};

        float a = std::atan2(u[2], u[0]) - std::atan2(w[2], w[0]);
        (void)azimuthHintDeg;  // az has no branch ambiguity (unlike el) - kept for symmetry/future use

        outAzimuthDeg = -a * 180.0f / (float)M_PI;
        outElevationDeg = -b * 180.0f / (float)M_PI;
        return true;
    }

    // Eye/target are used exactly as _orbit_C_tgt computes them - no Y-flip. An earlier version
    // flipped Y here (and correspondingly in buildPointCloudsThreadFunc()'s point cloud) to get
    // "+Y up" on screen without touching the server's own math - but flipping exactly ONE axis of
    // a right-handed frame makes it LEFT-handed, and mat4LookAt's right = cross(forward, up)
    // construction assumes right-handed input; fed a left-handed one, its computed "right" vector
    // points the wrong way on screen, mirroring the whole render on X (confirmed directly: "the
    // pointcloud is flipped on the x axis"). Passing worldUp=(0,-1,0) instead - "the server frame's
    // own DOWN direction is what should render as up" - achieves the same Y-up screen orientation
    // without touching a single coordinate, so handedness (and therefore left/right) stays correct.
    Mat4 getViewMatrix() const {
        float eye[3], target[3];
        getEyeAndTargetServerFrame(eye, target);
        const float worldUp[3] = {0.0f, -1.0f, 0.0f};
        return mat4LookAt(eye, target, worldUp);
    }

    // Perspective, matching the pinhole camera the point cloud was itself unprojected with
    // (buildPointCloudsThreadFunc in CameraPathEditor.cpp uses this same hfovDeg for its focal
    // length) - orthographic was tried instead to sidestep an apparent depth-dependent frustum
    // mismatch, but that turned out to be a red herring (the real bug was drawViewport() never
    // binding the default framebuffer, so nothing painted to the screen regardless of projection).
    // With perspective, near content naturally reads larger/wider and far content smaller, the
    // same way the source video's own camera saw it - orthographic showed everything at a uniform
    // scale regardless of depth, which is why the framed view looked wider than the original video.
    // hfovDeg is horizontal FOV (matches CrossViewWarp's own convention) - convert to the vertical
    // FOV mat4Perspective expects via the aspect ratio.
    Mat4 getProjectionMatrix(float aspect, float nearZ, float farZ) const {
        float hfovRad = hfovDeg * (float)M_PI / 180.0f;
        float vfovRad = 2.0f * std::atan(std::tan(hfovRad * 0.5f) / aspect);
        return mat4Perspective(vfovRad, aspect, nearZ, farZ);
    }
};

// Overshoot compensation for the LTX CrossView-Warp LoRA's measured underdelivery of requested
// camera motion (confirmed via direct pixel measurement against generated clips). Not currently
// wired into any live-generated pose - workflows/ltx_*/camera_warp.json (Camera Warp, "in the
// shadow", not yet registered as a preset) uses static identity-pose placeholders instead, since
// no UI drives its camera parameters yet. Kept shared here (not local to a splicing function, now
// removed) because CameraPathEditor.cpp's
// autoEaseDistanceForHoles() needs the EXACT same boost to check hole risk against the pose that
// will actually be generated - checking the raw, unboosted editor values was confirmed to badly
// underestimate real hole risk (7% estimated for a raw pose whose BOOSTED version rendered at
// 80%+ magenta on the actual server). Keeping one definition means the two can never drift apart.
static constexpr float kCameraMotionMultiplier = 1.67f;

inline float boostAzimuth(float deg) {
    return std::max(-180.0f, std::min(180.0f, deg * kCameraMotionMultiplier));
}
inline float boostElevation(float deg) {
    return std::max(-90.0f, std::min(90.0f, deg * kCameraMotionMultiplier));
}
inline float boostDistance(float dist) {
    float boosted = 1.0f + (dist - 1.0f) * kCameraMotionMultiplier;
    return std::max(0.1f, std::min(3.0f, boosted));
}

#endif // CAMERA3D_H
