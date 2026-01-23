/* SimShip by Edouard Halbert
This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/ */

#pragma once

#include <string>
#include <vector>
#include <map>

// glm
#include <glm/glm.hpp>
// pugixml
#include "pugixml/pugixml.hpp"
#ifdef _DEBUG
#pragma comment(lib, "pugixml/Debug/pugixml.lib")
#else
#pragma comment(lib, "pugixml/Release/pugixml.lib")
#endif

#include "Utility.h"
#include "Camera.h"
#include "Shader.h"
#include "Model.h"

using namespace std;
using namespace glm;

extern SoundManager*    g_SoundMgr;
extern bool             g_bPause;
extern Camera           g_Camera;

enum class eNavigationalStatus : uint8_t 
{
    UnderWayUsingEngine                         = 0,
    AtAnchor                                    = 1,
    NotUnderCommand                             = 2,
    RestrictedManeuverability                   = 3,
    ConstrainedByDraught                        = 4,
    Moored                                      = 5,
    Aground                                     = 6,
    EngagedInFishing                            = 7,
    UnderWaySailing                             = 8,
    Reserved9                                   = 9,
    Reserved10                                  = 10,
    PowerDrivenTowingAstern                     = 11,
    PowerDrivenPushingAheadOrTowingAlongside    = 12,
    Reserved13                                  = 13,
    AIS_SART_Active                             = 14,
    Undefined                                   = 15
};
enum eShipType
{
    not_available = 0,
    // 1..19 reserved for future use
    wing_in_ground = 20,
    wing_in_ground_hazardous_cat_a = 21,
    wing_in_ground_hazardous_cat_b = 22,
    wing_in_ground_hazardous_cat_c = 23,
    wing_in_ground_hazardous_cat_d = 24,
    // 25..29 WIG reserved for future use
    fishing = 30,
    towing = 31,
    towing_large = 32, // exceeds 200m length or 25m breadth
    dredging_or_underwater_ops = 33,
    diving_ops = 34,
    military_ops = 35,
    sailing = 36,
    pleasure_craft = 37,
    // 38..39 reserved
    high_speed_craft = 40,
    high_speed_craft_hazardous_cat_a = 41,
    high_speed_craft_hazardous_cat_b = 42,
    high_speed_craft_hazardous_cat_c = 43,
    high_speed_craft_hazardous_cat_d = 44,
    // 45..48 HSC reserved for future use
    high_speed_craft_no_info = 49,
    pilot_vessel = 50,
    search_and_rescue_vessel = 51,
    tug = 52,
    port_tender = 53,
    anti_pollution_equipment = 54,
    law_enforcement = 55,
    // 56..57 spare, local vessel
    medical_transport = 58,
    noncombatant = 59,
    passenger = 60,
    passenger_hazardous_cat_a = 61,
    passenger_hazardous_cat_b = 62,
    passenger_hazardous_cat_c = 63,
    passenger_hazardous_cat_d = 64,
    // 65..68 Passenger reserved for future use
    passenger_no_info = 69,
    cargo = 70,
    cargo_hazardous_cat_a = 71,
    cargo_hazardous_cat_b = 72,
    cargo_hazardous_cat_c = 73,
    cargo_hazardous_cat_d = 74,
    // 75..78 Cargo reserved for future use
    cargo_no_info = 79,
    tanker = 80,
    tanker_hazardous_cat_a = 81,
    tanker_hazardous_cat_b = 82,
    tanker_hazardous_cat_c = 83,
    tanker_hazardous_cat_d = 84,
    // 85..88 Tanker reserved for future use
    tanker_no_info = 89,
    other = 90,
    other_hazardous_cat_a = 91,
    other_hazardous_cat_b = 92,
    other_hazardous_cat_c = 93,
    other_hazardous_cat_d = 94,
    // 95..98 Other type reserved for future use
    other_no_info = 99,
};

class Traffic
{
public:
    struct TrafficPoint
    {
        vec2    position;
        float   speed;
        bool    bArc = false;
    };
    vector<TrafficPoint>vRoute;
    unique_ptr<Model>	Ship;

    vec2        CurPos = vec2(0.0f);
    float       CurYaw = 0.0f;
    bool        bContour = true;
    bool        bSound = true;

    Traffic(const pugi::xml_node& node)
    {
        // Load xml data
        auto modelNode = node.child(L"Model");
        if (modelNode)
            ModelName = modelNode.child_value();
 
        auto nameNode = node.child(L"Name");
        if (nameNode)
            Name = nameNode.child_value();
        
        wstring wtype;
		auto shipTypeNode = node.child(L"Type");
		if (shipTypeNode)
            wtype = shipTypeNode.child_value();
		string type = wstring_to_utf8(wtype);
        ShipType = stringToShipType(type);

        auto mmsiNode = node.child(L"MMSI");
        if (mmsiNode)
            MMSI = mmsiNode.child_value();

		auto callSignNode = node.child(L"CallSign");
		if (callSignNode)
			CallSign = callSignNode.child_value();
		
        auto toBowNode = node.child(L"to_bow");
		if (toBowNode)
			ToBow = toBowNode.attribute(L"value").as_float();
		
        auto toSternNode = node.child(L"to_stern");
		if (toSternNode)
			ToStern = toSternNode.attribute(L"value").as_float();
		
        auto toPortNode = node.child(L"to_port");
		if (toPortNode)
			ToPort = toPortNode.attribute(L"value").as_float();
		
        auto toStarboardNode = node.child(L"to_starboard");
		if (toStarboardNode)
			ToStarboard = toStarboardNode.attribute(L"value").as_float();
		
        auto draughtNode = node.child(L"Draught");
		if (draughtNode)
			Draught = draughtNode.attribute(L"value").as_float();

        auto epfdNode = node.child(L"epfd");
		if (epfdNode)
			EPFD = epfdNode.attribute(L"value").as_int();

        float lastSpeed = 0.0f;
        for (auto ptNode : node.children(L"Point"))
        {
            TrafficPoint pt;
            float x = ptNode.attribute(L"lon").as_float();
            float y = ptNode.attribute(L"lat").as_float();
            vec3 pos = lonlat_to_opengl(x, y);
            pt.position = vec2(pos.x, pos.z);
            // Check if the speed attribute exists and is not null
            if (ptNode.attribute(L"speed"))
            {
                pt.speed = ptNode.attribute(L"speed").as_float();
                lastSpeed = pt.speed; // update to the latest known speed
            }
            else
            {
                pt.speed = lastSpeed;  // resume last known speed
            }
            if (ptNode.attribute(L"arc"))
                pt.bArc = (bool)ptNode.attribute(L"arc").as_int();
            vRoute.emplace_back(pt);
        }
        mvLightPositions.clear();
        mvLightColors.clear();
        for (pugi::xml_node light : node.child(L"Lights").children(L"Light")) 
        {
            // Lire la position
            pugi::xml_node posNode = light.child(L"Position");
            float x = posNode.attribute(L"x").as_float();
            float y = posNode.attribute(L"y").as_float();
            float z = posNode.attribute(L"z").as_float();
            mvLightPositions.push_back(glm::vec3(x, y, z));

            // Lire la couleur
            pugi::xml_node colorNode = light.child(L"Color");
            float r = colorNode.attribute(L"r").as_float();
            float g = colorNode.attribute(L"g").as_float();
            float b = colorNode.attribute(L"b").as_float();
            mvLightColors.push_back(glm::vec3(r, g, b));
        }
        ConstructSegmentsFromRoute();

        // Init the model
        Ship = make_unique<Model>(wstring_to_utf8(ModelName));
        CurPos = vRoute[0].position;
        CurYaw = std::atan2(-(vRoute[1].position.y - vRoute[0].position.y), vRoute[1].position.x - vRoute[0].position.x);

        // Build VAO
        BuildSegmentsVAO();
        BuildBezierVAO();
        mShaderUnicolor = make_unique<Shader>("Resources/Misc/unicolor.vert", "Resources/Misc/unicolor.frag");

        // Lights
        mShaderLight = make_unique<Shader>("Resources/Misc/light.vert", "Resources/Misc/light.frag");
        InitBillboard();

        // Init the sound of the engine
        wstring wsound;
        auto soundNode = node.child(L"Sound");
        if (soundNode)
            wsound = soundNode.child_value();
        string sound = wstring_to_utf8(wsound);
        mSoundThrust = make_unique<Sound>(sound);
        mSoundThrust->setVolume(0.25f);
		vec3 pos = vec3(CurPos.x, 0.0f, CurPos.y);
        mSoundThrust->setPosition(pos);
        mSoundThrust->setLooping(true);
        mSoundThrust->adjustDistances();
        if (bSound)
            mSoundThrust->play();

        auto speedmaxNode = node.child(L"SpeedMax");
        if (speedmaxNode)
            mSpeedMaxKt = speedmaxNode.attribute(L"value").as_float();
    }
    ~Traffic()
    {
		mSoundThrust->stop();
        mSoundThrust.reset();

		glDeleteVertexArrays(1, &mVaoSegments);
        glDeleteVertexArrays(1, &mVaoBezier);
		glDeleteVertexArrays(1, &mQuadVAO);
		
        mShaderUnicolor.reset();
        mShaderLight.reset();
    }

    void Update(float dt)
    {
		float temp_dt = dt;

        if (mCurSegmentIndex >= mvSegments.size())
            return;
        
        float speed = 0.0f;

        while (dt > 0 && mCurSegmentIndex < mvSegments.size())
        {
            Segment& seg = mvSegments[mCurSegmentIndex];

            speed = (seg.type == SegmentType::Line) ? glm::mix(seg.line.speedStart, seg.line.speedEnd, mCurT) : glm::mix(seg.bezier.speedStart, seg.bezier.speedEnd, mCurT);

            float segmentLength = (seg.type == SegmentType::Line) ? glm::distance(seg.line.start, seg.line.end) : ApproximateBezierLength(seg.bezier.P0, seg.bezier.P1, seg.bezier.P2, seg.bezier.P3);

            float distRemaining = (1.0f - mCurT) * segmentLength;
            float distToTravel = speed * dt;

            if (distToTravel + 1e-6 < distRemaining)
            {
                float paramDelta = distToTravel / segmentLength;
                mCurT += paramDelta;
                dt = 0.0f; // everything consumed
            }
            else
            {
                dt -= distRemaining / speed; // adjust the time spent
                mCurT = 0.0f;
                mCurSegmentIndex++;
                if (mCurSegmentIndex >= (int)mvSegments.size())
                {
                    mCurSegmentIndex = (int)mvSegments.size() - 1;
                    dt = 0.0f;
                    break;
                }
            }
        }

        // Calculating position and orientation on the current segment
        const Segment& curSeg = mvSegments[mCurSegmentIndex];
        if (curSeg.type == SegmentType::Line)
        {
            CurPos = glm::mix(curSeg.line.start, curSeg.line.end, mCurT);
            mTangent = glm::normalize(curSeg.line.end - curSeg.line.start);
            CurYaw = std::atan2(-mTangent.y, mTangent.x);
        }
        else
        {
            CurPos = EvaluateBezier(curSeg.bezier.P0, curSeg.bezier.P1, curSeg.bezier.P2, curSeg.bezier.P3, mCurT);
            mTangent = EvaluateBezierDerivative(curSeg.bezier.P0, curSeg.bezier.P1, curSeg.bezier.P2, curSeg.bezier.P3, mCurT);
            mTangent = glm::normalize(mTangent);
            CurYaw = std::atan2(-mTangent.y, mTangent.x);
        }
		vec2 lonlat = opengl_to_lonlat(CurPos.x, CurPos.y);
		Longitude = lonlat.x;
		Latitude = lonlat.y;
		COG = get_hdg_from_yaw(CurYaw);
        SOG = ms_to_knot(speed);

        float deltaYaw = CurYaw - PrevYaw;
        while (deltaYaw > M_PI) deltaYaw -= 2.0f * M_PI;
        while (deltaYaw < -M_PI) deltaYaw += 2.0f * M_PI;
        float deltaYawDeg = deltaYaw * 180.0f / static_cast<float>(M_PI);
        ROT = (deltaYawDeg / temp_dt) * 60.0f;
        PrevYaw = CurYaw;

        UpdateSound();
    }
    string NMEA_AIVDM_1()
    {
        string bin;
        auto append_bits = [&](uint64_t value, int bits) {
            for (int i = bits - 1; i >= 0; --i)
                bin += ((value >> i) & 1) ? '1' : '0';
            };

        // Construction of the binary message
        append_bits(1, 6);                          // Message Type
        append_bits(0, 2);                          // Repeat indicator
        append_bits(stoull(MMSI), 30);              // MMSI
        append_bits(static_cast<uint8_t>(eNavigationalStatus::UnderWayUsingEngine), 4);
        
        float rot = ROT;
        append_bits(static_cast<uint8_t>(rot), 8);  // ROT
        
        float sog = SOG;
        uint16_t sog_val = (sog < 0) ? 1023 : std::min(static_cast<uint16_t>(sog * 10), (uint16_t)1023);
        append_bits(sog_val, 10);
        
        append_bits(1, 1);                          // Position accuracy
		
        float longitude = Longitude;
        int32_t lon_val = static_cast<int32_t>(longitude * 600000);
        if (lon_val < 0) lon_val = (1 << 28) + lon_val;
        append_bits(lon_val, 28);
        
        float latitude = Latitude;
        int32_t lat_val = static_cast<int32_t>(latitude * 600000);
        if (lat_val < 0) lat_val = (1 << 27) + lat_val;
        append_bits(lat_val, 27);
        
        float cog = COG;
        uint16_t cog_val = std::min(static_cast<uint16_t>(cog * 10), (uint16_t)3599);
        append_bits(cog_val, 12);
        
        float hdg = COG;// 270.0f;
        uint16_t heading_val = (hdg >= 0 && hdg <= 359) ? static_cast<uint16_t>(hdg) : 511;
        append_bits(heading_val, 9);
        
        // Timestamp
        time_t now = std::time(nullptr);            // obtenir le temps actuel en secondes depuis Epoch
        tm* localTime = std::localtime(&now);       // convertir en temps local
        append_bits(localTime->tm_sec, 6);          // Second of UTC timestamp
        
        append_bits(3, 2);                          // Maneuver Indicator
        append_bits(0, 3);                          // Spare (not used)
        append_bits(0, 1);                          // RAIM flag
        append_bits(49287, 19);                     // Communication state (Radio status)

        // Encode in ASCII 6-bit AIS
        string payload_ascii = encodeAISPayloadToNMEAASCII(bin);

        // Get the sentence AIVDM
        int total_fragments = 1;
        int fragment_number = 1;
        int sequence_id = 0;
        char channel = 'A';

        string sentence;
        
        if (sequence_id > 0)
            sentence = "AIVDM," + to_string(total_fragments) + "," + to_string(fragment_number) + "," + to_string(sequence_id) + "," + channel + "," + payload_ascii + ",0";
        else
            sentence = "AIVDM," + to_string(total_fragments) + "," + to_string(fragment_number) + ",," + channel + "," + payload_ascii + ",0";

        // Checksum
        unsigned char checksum = calculate_checksum(sentence);

        // Final sentence
        char buf[3];
        snprintf(buf, sizeof(buf), "%02X", (int)checksum);
        string final_sentence = "!" + sentence + "*" + string(buf) + "\r\n";
        return final_sentence;
    }
    string NMEA_AIVDM_5()
    {
        string bin;
        auto append_bits = [&](uint64_t value, int bits) {
            for (int i = bits - 1; i >= 0; --i)
                bin += ((value >> i) & 1) ? '1' : '0';
            };

        auto to_uppercase = [](string& s) {
            std::transform(s.begin(), s.end(), s.begin(),
                [](unsigned char c) { return std::toupper(c); });
            };

        // Construction du message type 5 (424 bits)
        append_bits(5, 6);                          // Message Type = 5
        append_bits(0, 2);                          // Repeat indicator
        append_bits(stoull(MMSI), 30);              // MMSI
        append_bits(0, 2);                          // AIS version
        append_bits(IMO, 30);                       // IMO number

        // CallSign codé en 6 bits, 7 caractères max
        string callsign = wstring_to_utf8(CallSign);
        to_uppercase(callsign);
        for (int i = 0; i < 7; i++) {
            char c = (i < callsign.size()) ? callsign[i] : ' ';
            uint8_t sixbit = encodeAISChar6(c);
            append_bits(sixbit, 6);
        }
        
        // Nom du navire codé en 6 bits, 20 caractères max
		string name = wstring_to_utf8(Name);
        to_uppercase(name);
        for (int i = 0; i < 20; i++) {
            char c = (i < name.size()) ? name[i] : ' ';
            uint8_t sixbit = encodeAISChar6(c);
            append_bits(sixbit, 6);
        }

        append_bits(ShipType, 8); // Ship type

        // Dimensions
        append_bits(ToBow, 9);
        append_bits(ToStern, 9);
        append_bits(ToPort, 6);
        append_bits(ToStarboard, 6);
        
        append_bits(EPFD, 4);                       // EPFD fix type

        // ETA
        append_bits(0, 4);                          // Month
        append_bits(0, 5);                          // Day
        append_bits(0, 5);                          // Hour
        append_bits(0, 6);                          // Minute

        // Draught
        append_bits(static_cast<uint8_t>(Draught * 10.0f / 4), 8);  // Draught
        
        // Destination codés en 6 bits, 20 caractères max.
        string destination = "FR-HOUAT";
        for (int i = 0; i < 20; i++) {
            char c = (i < destination.size()) ? destination[i] : ' ';
            uint8_t sixbit = encodeAISChar6(c);
            append_bits(sixbit, 6);
        }

        append_bits(1, 1);                          // DTE  0=Data terminal ready, 1=Not ready (default).
        append_bits(0, 1);                          // Spare (Not used)

        // Gestion de la fragmentation : message type 5 est en 2 fragments
        int total_fragments = 2;
        int sequence_id = 0;
        char channel = 'A';

        // Découper bin en 2 parties
        string bin1 = bin.substr(0, 168);
        string bin2 = bin.substr(168);

        string payload1 = encodeAISPayloadToNMEAASCII(bin1);
        string payload2 = encodeAISPayloadToNMEAASCII(bin2);

        // Générer les phrases NMEA
        string sentence1 = "AIVDM," + to_string(total_fragments) + ",1," + to_string(sequence_id) + "," + channel + "," + payload1 + ",0";
        string sentence2 = "AIVDM," + to_string(total_fragments) + ",2," + to_string(sequence_id) + "," + channel + "," + payload2 + ",0";

        unsigned char checksum1 = calculate_checksum(sentence1);
        unsigned char checksum2 = calculate_checksum(sentence2);

        char buf1[3], buf2[3];
        snprintf(buf1, sizeof(buf1), "%02X", checksum1);
        snprintf(buf2, sizeof(buf2), "%02X", checksum2);

        string final_sentence = "!" + sentence1 + "*" + string(buf1) + "\r\n" + "!" + sentence2 + "*" + string(buf2) + "\r\n";
        
        return final_sentence;
    }

    void RenderSegments(Camera& camera)
    {
        if (!bContour)
            return;

        mShaderUnicolor->use();

        mShaderUnicolor->setMat4("view", camera.GetView());
        mShaderUnicolor->setMat4("projection", camera.GetProjection());
        mat4 model = glm::translate(mat4(1.0f), vec3(0.0f, 0.4f, 0.0f));
        mShaderUnicolor->setMat4("model", model);
        mShaderUnicolor->setVec3("lineColor", vec3(0.0f, 1.0f, 0.0f));

        glBindVertexArray(mVaoSegments);
        glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(mIndicesSegments));
        glBindVertexArray(0);
    }
    void RenderBezier(Camera& camera)
    {
        if (!bContour)
            return;

        mShaderUnicolor->use();

        mShaderUnicolor->setMat4("view", camera.GetView());
        mShaderUnicolor->setMat4("projection", camera.GetProjection());
        mat4 model = glm::translate(mat4(1.0f), vec3(0.0f, 0.5f, 0.0f));
        mShaderUnicolor->setMat4("model", model);
        mShaderUnicolor->setVec3("lineColor", vec3(1.0f, 1.0f, 1.0f));

        glBindVertexArray(mVaoBezier);
        glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(mIndicesBezier));
        glBindVertexArray(0);
    }
    void RenderOneLight(Camera& camera, int i)
    {
        glBindVertexArray(mQuadVAO);
        
        mShaderLight->use();            // Misc/light.vert, Misc/light.frag
        vec3 p = vec3(CurPos.x, 0.0f, CurPos.y);
        mat world = glm::translate(mat4(1.0f), p);
        world = glm::rotate(world, CurYaw, vec3(0.0f, 1.0f, 0.0f));
        mat4 model = glm::translate(world, mvLightPositions[i]);

        vec3 camRight = vec3(camera.GetView()[0][0], camera.GetView()[1][0], camera.GetView()[2][0]);
        vec3 camUp = vec3(camera.GetView()[0][1], camera.GetView()[1][1], camera.GetView()[2][1]);

        model[0] = vec4(camRight, 0.0f);
        model[1] = vec4(camUp, 0.0f);

		float dCameraToLight = glm::length(camera.GetPosition() - p);
        // Distance limits and scales
        const float dMin = 0.5f * 1852.0f;      // 1/2 NM
        const float dMax = 9.0f * 1852.0f;      // 9 NM
        const float sMin = 2.0f;
        const float sMax = 20.0f;
        // Linear interpolation with clamp
        float t = (dCameraToLight - dMin) / (dMax - dMin);
        t = glm::clamp(t, 0.0f, 1.0f);          // t in [0,1]
        float scale = sMin + t * (sMax - sMin); // scale in [2,20]
        model = glm::scale(model, glm::vec3(scale));

        mShaderLight->setMat4("model", model);
        mShaderLight->setMat4("view", camera.GetView());
        mShaderLight->setMat4("projection", camera.GetProjection());
        mShaderLight->setVec3("lightColor", mvLightColors[i]);
        mShaderLight->setFloat("lightIntensity", 1.0f);
        mShaderLight->setFloat("starIntensity", 0.1f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
        glBindVertexArray(0);
    }
    void RenderLights(Camera& camera)
    {
        // Visibility of navigation lights
        vec3 shipPos = vec3(CurPos.x, 0.0f, CurPos.y);
        vec3 shipForward = vec3(mTangent.x, 0.0f, mTangent.y);
        vec3 cameraToShip = camera.GetPosition() - shipPos;
        cameraToShip.y = 0.0f;
        float angleDeg = degrees(orientedAngle(glm::normalize(shipForward), glm::normalize(cameraToShip), vec3(0.0f, 1.0f, 0.0f)));
        
        // From starboard
        if (angleDeg > -112.5f && angleDeg < -3.0f)
			RenderOneLight(camera, 1);    // Green
		// From ahead
        else if (angleDeg >= -3.0f && angleDeg <= 3.0f)
        {
            RenderOneLight(camera, 0);    // Red
            RenderOneLight(camera, 1);    // Green
        }
		// From port
        else if (angleDeg > 3.0f && angleDeg < 112.5f)
            RenderOneLight(camera, 0);    // Red
		// From astern
        else
            RenderOneLight(camera, 2);    // White
		
        // Masthead and stern lights
        RenderOneLight(camera, 3);        // White high
        if (mvLightPositions.size() > 4)
            RenderOneLight(camera, 4);    // White high
    }

private:
    struct SegmentBezier
    {
        vec2 P0, P1, P2, P3;
        float speedStart;
        float speedEnd;
    };
    struct SegmentLine
    {
        vec2 start, end;
        float speedStart;
        float speedEnd;
    };
    enum class SegmentType { Line, Bezier };
    struct Segment
    {
        SegmentType type;
        union
        {
            SegmentLine line;
            SegmentBezier bezier;
        };
    };
    
    // Model 3D
    wstring             ModelName;
    //
	wstring             Name;
    wstring             CallSign;
	int                 IMO = 0;
	int				    ToBow = 0;          // In meters
	int				    ToStern = 0;        // In meters
	int				    ToPort = 0;         // In meters
	int				    ToStarboard = 0;    // In meters
	float			    Draught = 0.0f;     // In meters
	int				    EPFD = 0;           // 0=Undefined, 1=GPS, 2=GLONASS, 3=Combined, 4=Loran-C, 5=Chayka, 6=Integrated Navigation System, 7=Surveyed
    // Route
    vector<Segment>     mvSegments;
    float               mCurT = 0.0f;
    int                 mCurSegmentIndex = 0;
    vec2                mTangent;
    // Route VAO 
    GLuint		        mVaoSegments = 0;       // Segments
    int			        mIndicesSegments = 0;
    GLuint		        mVaoBezier = 0;         // Bézier curves
    int			        mIndicesBezier = 0;
    unique_ptr<Shader>  mShaderUnicolor;        // Shader
    // Lights
    GLuint              mQuadVAO = 0;
    unique_ptr<Shader>  mShaderLight;
    vector<vec3>        mvLightPositions;
    vector<vec3>        mvLightColors;

    unique_ptr<Sound>	mSoundThrust;
	float			    mSpeedMaxKt = 10.0f;

    wstring             MMSI;
	float               SOG;        // Speed over ground in knots
	float 			    COG;        // Course over ground in degrees
	float               Longitude;
	float               Latitude;
	float			    ROT;        // Rate of turn      
    float               PrevYaw;
    eShipType           ShipType;

    eShipType stringToShipType(const std::string& typeStr)
    {
        static const std::unordered_map<std::string, eShipType> mapType = {
            {"not_available", not_available},
            {"wing_in_ground", wing_in_ground},
            {"wing_in_ground_hazardous_cat_a", wing_in_ground_hazardous_cat_a},
            {"wing_in_ground_hazardous_cat_b", wing_in_ground_hazardous_cat_b},
            {"wing_in_ground_hazardous_cat_c", wing_in_ground_hazardous_cat_c},
            {"wing_in_ground_hazardous_cat_d", wing_in_ground_hazardous_cat_d},
            {"fishing", fishing},
            {"towing", towing},
            {"towing_large", towing_large},
            {"dredging_or_underwater_ops", dredging_or_underwater_ops},
            {"diving_ops", diving_ops},
            {"military_ops", military_ops},
            {"sailing", sailing},
            {"pleasure_craft", pleasure_craft},
            {"high_speed_craft", high_speed_craft},
            {"high_speed_craft_hazardous_cat_a", high_speed_craft_hazardous_cat_a},
            {"high_speed_craft_hazardous_cat_b", high_speed_craft_hazardous_cat_b},
            {"high_speed_craft_hazardous_cat_c", high_speed_craft_hazardous_cat_c},
            {"high_speed_craft_hazardous_cat_d", high_speed_craft_hazardous_cat_d},
            {"high_speed_craft_no_info", high_speed_craft_no_info},
            {"pilot_vessel", pilot_vessel},
            {"search_and_rescue_vessel", search_and_rescue_vessel},
            {"tug", tug},
            {"port_tender", port_tender},
            {"anti_pollution_equipment", anti_pollution_equipment},
            {"law_enforcement", law_enforcement},
            {"medical_transport", medical_transport},
            {"noncombatant", noncombatant},
            {"passenger", passenger},
            {"passenger_hazardous_cat_a", passenger_hazardous_cat_a},
            {"passenger_hazardous_cat_b", passenger_hazardous_cat_b},
            {"passenger_hazardous_cat_c", passenger_hazardous_cat_c},
            {"passenger_hazardous_cat_d", passenger_hazardous_cat_d},
            {"passenger_no_info", passenger_no_info},
            {"cargo", cargo},
            {"cargo_hazardous_cat_a", cargo_hazardous_cat_a},
            {"cargo_hazardous_cat_b", cargo_hazardous_cat_b},
            {"cargo_hazardous_cat_c", cargo_hazardous_cat_c},
            {"cargo_hazardous_cat_d", cargo_hazardous_cat_d},
            {"cargo_no_info", cargo_no_info},
            {"tanker", tanker},
            {"tanker_hazardous_cat_a", tanker_hazardous_cat_a},
            {"tanker_hazardous_cat_b", tanker_hazardous_cat_b},
            {"tanker_hazardous_cat_c", tanker_hazardous_cat_c},
            {"tanker_hazardous_cat_d", tanker_hazardous_cat_d},
            {"tanker_no_info", tanker_no_info},
            {"other", other},
            {"other_hazardous_cat_a", other_hazardous_cat_a},
            {"other_hazardous_cat_b", other_hazardous_cat_b},
            {"other_hazardous_cat_c", other_hazardous_cat_c},
            {"other_hazardous_cat_d", other_hazardous_cat_d},
            {"other_no_info", other_no_info}
        };

        auto it = mapType.find(typeStr);
        if (it != mapType.end())
            return it->second;
        return not_available; // valeur par défaut si non trouvée
    }
    inline uint8_t encodeAISChar6(char c) 
    {
        if (c == ' ') return 32;           // espace codé 32
        if (c == '@') return 0;            // '@' codé 0
        if (c >= 'A' && c <= 'Z')          // Lettres majuscules
            return c - 'A' + 1;
        if (c >= '0' && c <= '9')          // Chiffres '0' -> 16, '1' -> 17, ..., '9' -> 25
            return c - '0' + 48;
        if (c >= '[' && c <= '_')          // Caractères spéciaux
            return c - '[' + 27;
        return 0;                         // Sinon code 0
    }
    unsigned char calculate_checksum(const string& sentence)
    {
        // Calculate the NMEA XOR checksum on the string between $ and *
        unsigned char checksum = 0;
        for (size_t i = 0; i < sentence.size(); ++i)
            checksum ^= sentence[i];

        return checksum;
    }
    string encodeAISPayloadToNMEAASCII(const string& bin)
    {
        /*
        This code divides the binary string into groups of 6 bits.
        For each group, it converts the 6 bits into a value between 0 and 63.
        If the value is less than 40, it adds 48 (the character '0') to obtain the corresponding ASCII character.
        If the value is 40 or more, it adds 8 in addition to 48 to skip the unused range between 88 and 95 and reach characters from '`' to 'w'.
        */
        string asciiPayload;
        for (size_t i = 0; i < bin.size(); i += 6) 
        {
            unsigned char val = 0;
            for (int j = 0; j < 6; ++j) 
            {
                val <<= 1;
                if (i + j < bin.size())
                    val |= (bin[i + j] == '1');
            }
            val &= 0x3F; // 6 bits max

            // Official table NMEA
            if (val < 40)
                asciiPayload += static_cast<char>('0' + val);
            else
                asciiPayload += static_cast<char>(val + 8 + 48); // '`' à 'w'
        }
        return asciiPayload;
    }

    vec2 EvaluateBezier(const vec2& P0, const vec2& P1, const vec2& P2, const vec2& P3, float t)
    {
        // Evaluates a point on a cubic Bézier curve with parameter t (0<=t<=1)
        
        float u = 1.0f - t;
        float tt = t * t;
        float uu = u * u;
        float uuu = uu * u;
        float ttt = tt * t;

        vec2 point = uuu * P0;           // (1-t)^3 * P0
        point += 3 * uu * t * P1;        // 3*(1-t)^2*t*P1
        point += 3 * u * tt * P2;        // 3*(1-t)*t^2*P2
        point += ttt * P3;               // t^3 * P3

        return point;
    }
    vec2 EvaluateBezierDerivative(const vec2& P0, const vec2& P1, const vec2& P2, const vec2& P3, float t)
    {
        // Evaluates the derivative (tangent) of a cubic Bézier curve with parameter t
        
        float u = 1.0f - t;

        vec2 derivative =
            3.0f * u * u * (P1 - P0) +
            6.0f * u * t * (P2 - P1) +
            3.0f * t * t * (P3 - P2);

        return derivative;
    }
    float ApproximateBezierLength(const vec2& P0, const vec2& P1, const vec2& P2, const vec2& P3, int steps = 20)
    {
        // Numerical approximation of the length of a cubic Bézier curve via subdivision into linear segments and sum of Euclidean distances
        
        float length = 0.0f;
        vec2 prevPoint = P0;

        for (int i = 1; i <= steps; ++i)
        {
            float t = i / (float)steps;
            vec2 currentPoint = EvaluateBezier(P0, P1, P2, P3, t);
            length += glm::distance(currentPoint, prevPoint);
            prevPoint = currentPoint;
        }

        return length;
    }
    void ConstructSegmentsFromRoute()
    {
        mvSegments.clear();

        for (size_t i = 0; i < vRoute.size() - 1; ++i)
        {
            // If the following point is an arc (Bézier)
            if (vRoute[i].bArc && i > 0 && i + 2 < vRoute.size())
            {
                const vec2& A = vRoute[i - 1].position;
                const vec2& B = vRoute[i].position;
                const vec2& C = vRoute[i + 1].position;
                const vec2& D = vRoute[i + 2].position;

                float k = glm::length(C - B) * 0.5f;

                SegmentBezier bezierSeg;
                bezierSeg.P0 = B;
                bezierSeg.P3 = C;
                bezierSeg.P1 = B + k * glm::normalize(B - A);
                bezierSeg.P2 = C + k * glm::normalize(C - D);

                bezierSeg.speedStart = vRoute[i].speed;
                bezierSeg.speedEnd = vRoute[i + 1].speed;

                Segment seg;
                seg.type = SegmentType::Bezier;
                seg.bezier = bezierSeg;
                mvSegments.push_back(seg);
            }
            else
            {
                SegmentLine lineSeg;
                lineSeg.start = vRoute[i].position;
                lineSeg.end = vRoute[i + 1].position;
                lineSeg.speedStart = vRoute[i].speed;
                lineSeg.speedEnd = vRoute[i + 1].speed;

                Segment seg;
                seg.type = SegmentType::Line;
                seg.line = lineSeg;
                mvSegments.push_back(seg);
            }
        }
    }
    void BuildSegmentsVAO()
    {
        vector<vec3> contour;
        for (const auto& p : vRoute)
        {
            contour.emplace_back(p.position.x, 0.0f, p.position.y);
        }

        if (mVaoSegments == 0)
            glGenVertexArrays(1, &mVaoSegments);
        GLuint mVboSegments = 0;
        glGenBuffers(1, &mVboSegments);
        glBindVertexArray(mVaoSegments);
        glBindBuffer(GL_ARRAY_BUFFER, mVboSegments);
        glBufferData(GL_ARRAY_BUFFER, contour.size() * sizeof(vec3), contour.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
        glBindVertexArray(0);

        mIndicesSegments = static_cast<int>(contour.size());
    }
    void BuildBezierVAO()
    {
        vector<vec3> contour;

        for (const auto& seg : mvSegments)
        {
            if (seg.type == SegmentType::Line)
            {
                // Add the two points of the straight segment (start, end)
                contour.push_back(vec3(seg.line.start.x, 0.0f, seg.line.start.y));
                contour.push_back(vec3(seg.line.end.x, 0.0f, seg.line.end.y));
            }
            else if (seg.type == SegmentType::Bezier)
            {
                // Sample the Bézier curve and add the points
                int steps = 20; // or adjust according to precision
                for (int i = 0; i <= steps; ++i)
                {
                    float t = i / float(steps);
                    vec2 p = EvaluateBezier(seg.bezier.P0, seg.bezier.P1, seg.bezier.P2, seg.bezier.P3, t);
                    contour.push_back(vec3(p.x, 0.0f, p.y));
                }
            }
        }

        // VAO and VBO creation for the smoothed road
        if (mVaoBezier == 0)
            glGenVertexArrays(1, &mVaoBezier);
        GLuint mVboArc = 0;
        glGenBuffers(1, &mVboArc);

        glBindVertexArray(mVaoBezier);
        glBindBuffer(GL_ARRAY_BUFFER, mVboArc);
        glBufferData(GL_ARRAY_BUFFER, contour.size() * sizeof(vec3), contour.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
        glBindVertexArray(0);

        mIndicesBezier = static_cast<int>(contour.size());
    }
    void InitBillboard()
    {
        float quadVertices[] = {
            // positions   // UVs
            -0.5f,  0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.0f, 0.0f,
             0.5f,  0.5f,  1.0f, 1.0f,
             0.5f, -0.5f,  1.0f, 0.0f,
        };

        GLuint quadVBO = 0;

        glGenVertexArrays(1, &mQuadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(mQuadVAO);

        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        // position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        // UV attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0);
    }
    void UpdateSound()
    {
        bool sound = bSound && g_SoundMgr->bSound && !g_bPause;
        
        mSoundThrust->setPitch(2.0f + 0.25f * fabs(SOG) / mSpeedMaxKt);
        if (g_Camera.GetPosition().y < 0.0f)
            mSoundThrust->setVolume(0.02f + 0.25f * fabs(SOG) / mSpeedMaxKt);
        else
            mSoundThrust->setVolume(0.25f + 0.25f * fabs(SOG) / mSpeedMaxKt);
		vec3 shipPos = vec3(CurPos.x, 0.0f, CurPos.y);
        mSoundThrust->setPosition(shipPos);

        static bool bPause = false;
        if (!sound)
        {
            mSoundThrust->pause();
            bPause = true;
        }
        if (sound && bPause)
        {
            mSoundThrust->play();
            bPause = false;
        }
    }
};

class Traffics 
{
public:
    vector<Traffic*>vTraffics;
    bool			bLights = true;
	bool			bShowRoute = false; 

    Traffics(const string& filename)
    {
        LoadFromFile(filename);
        mShaderShip = make_unique<Shader>("Resources/Ship/ship.vert", "Resources/Ship/ship.frag");
        mTexEnvironment = make_unique<Texture>();
        mTexEnvironment->CreateFromDDSFile("Resources/Ocean/ocean_env.dds");
    }
    ~Traffics()
    {
        for (auto tr : vTraffics)
            delete tr;
    }

    bool LoadFromFile(const string& filename) 
    {
        pugi::xml_document doc;
        if (!doc.load_file(filename.c_str())) return false;
        
        auto root = doc.child(L"Traffics");
        if (!root) return false;
        
        vTraffics.clear();
        for (auto trafficNode : root.children(L"Traffic")) 
        {
            Traffic* tr = new Traffic(trafficNode);
            vTraffics.emplace_back(tr);
        }
        return true;
    }
    float smooth_dt(float newVal)
    {
        const int nbValues = 500;
        const int nbIgnored = 300;

        static vector<float> values(nbValues, 0.0f);
        static int index = 0;
        static bool filled = false;
        static int callCount = 0;
        static float sum = 0.0f;

        callCount++;

        if (callCount <= nbIgnored)
            return newVal;  // Ignore recording and sum on the first n calls

        if (filled)
            sum -= values[index];

        sum += newVal;
        values[index] = newVal;

        index = (index + 1) % nbValues;
        if (index == 0) filled = true;

        int count = filled ? nbValues : (index);
        return sum / count;
    }
    void Update(float time)
    {
        static float prevTime = 0.0f;
        float dt = smooth_dt(time - prevTime);
        prevTime = time;
        
        for (const auto& tr : vTraffics)
            tr->Update(dt);
    }
    string NMEA_AIVDM_1()
    {
		string nmeaData;
        for (const auto& tr : vTraffics)
        {
            string sentence = tr->NMEA_AIVDM_1();
            nmeaData += sentence;
        }
		return nmeaData;
	}
    string NMEA_AIVDM_5(int index)
    {
		if (index < 0 || index >= vTraffics.size())
            return "";
        
        return vTraffics[index]->NMEA_AIVDM_5();
    }

    void Render(Camera& camera, Sky* sky)
    {
        if (bShowRoute)
        {
            for (const auto& tr : vTraffics)
            {
                //tr->RenderSegments(camera);
                tr->RenderBezier(camera);
			}
        }

        mShaderShip->use();     // Ship/ship.vert, Ship/ship.frag
        mShaderShip->setVec3("light.position", sky->SunPosition);
        mShaderShip->setVec3("light.ambient", sky->SunAmbient);
        mShaderShip->setVec3("light.diffuse", sky->SunDiffuse);
        mShaderShip->setVec3("light.specular", sky->SunSpecular);
        mShaderShip->setVec3("viewPos", camera.GetPosition());
        mShaderShip->setFloat("exposure", sky->Exposure);
        mShaderShip->setMat4("view", camera.GetView());
        mShaderShip->setMat4("projection", camera.GetProjection());
        mShaderShip->setFloat("envmapFactor", 0.0f);
        mShaderShip->setInt("texture_diffuse1", 0);
        mShaderShip->setInt("envmap", 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP,mTexEnvironment->id);

        for (const auto& tr : vTraffics)
        {
            vec3 pos = vec3(tr->CurPos.x, 0.0f, tr->CurPos.y);
            mat4 model = glm::translate(mat4(1.0f), pos);
            model = glm::rotate(model, tr->CurYaw, vec3(0.0f, 1.0f, 0.0f));
            mShaderShip->setMat4("model", model);

            tr->Ship->Render(*mShaderShip);
        }
    }
    void RenderLights(Camera& camera, bool bLights = false)
    {
        if (bLights)
            for (const auto& tr : vTraffics)
                tr->RenderLights(camera);
	}

private:
    unique_ptr<Texture>	mTexEnvironment;
    unique_ptr<Shader>  mShaderShip;

};