/* SimShip by Edouard Halbert
This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/ */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <cstdio>

// OpenAL
#include <al.h>
#include <alc.h>
// glm
#include <glm/glm.hpp>

using namespace std;
using namespace glm;


class SoundManager 
{
private:
    static SoundManager * instance;
    ALCdevice           * device;
    ALCcontext          * context;
    map<string, ALuint>   buffers;

    SoundManager() 
    {
        initOpenAL();
    }

public:

    bool    bSound = true;

    static SoundManager* getInstance() 
    {
        if (!instance) 
            instance = new SoundManager();

        return instance;
    }
    ~SoundManager() 
    {
        cleanupOpenAL();
    }
    bool initOpenAL() 
    {
        device = alcOpenDevice(NULL);
        if (!device) return false;

        context = alcCreateContext(device, NULL);
        if (!context) return false;

        if (!alcMakeContextCurrent(context)) return false;

        // OpenAL offers several mitigation models
        alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);               
        //alDopplerFactor(1.0f); // Reduces the Doppler effect
        
        return true;
    }
    void cleanupOpenAL() 
    {
        for (auto& pair : buffers) 
            alDeleteBuffers(1, &pair.second);

        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
        alcCloseDevice(device);
    }

    ALuint loadSound(const string& fichier) 
    {
        if (buffers.find(fichier) != buffers.end()) 
            return buffers[fichier];

        ALuint buffer;
        if (loadWAV(fichier, buffer)) 
        {
            buffers[fichier] = buffer;
            return buffer;
        }
        return 0;
    }

    // Methods for the listener
    void setListenerPosition(float x, float y, float z) 
    {
        alListener3f(AL_POSITION, x, y, z);
    }
    void setListenerPosition(vec3 pos)
    {
        ALfloat listenerPos[] = { pos.x, pos.y, pos.z };
        alListenerfv(AL_POSITION, listenerPos);
    }
    void setListenerOrientation(float atX, float atY, float atZ, float upX, float upY, float upZ) 
    {
        ALfloat orientation[] = { atX, atY, atZ, upX, upY, upZ };
        alListenerfv(AL_ORIENTATION, orientation);
    }
    void setListenerOrientation(vec3 at, vec3 up) 
    {
        ALfloat listenerOri[] = { at.x, at.y, at.z, up.x, up.y, up.z };
        alListenerfv(AL_ORIENTATION, listenerOri);
    }

private:

    bool loadWAV(const string& fichier, ALuint& buffer) 
    {
        FILE* fp = fopen(fichier.c_str(), "rb");
        if (!fp)
            throw std::runtime_error("Impossible d'ouvrir le fichier " + fichier);

        char chunkID[4];
        uint32_t chunkSize;
        uint32_t taux_echantillonnage;
        uint16_t format, canaux, bits_par_echantillon;
        uint32_t dataSize = 0;

        // Read RIFF header
        fread(chunkID, sizeof(char), 4, fp);
        if (strncmp(chunkID, "RIFF", 4) != 0)
        {
            fclose(fp);
            throw std::runtime_error("Le fichier n'est pas au format WAV");
        }

        fread(&chunkSize, sizeof(uint32_t), 1, fp);
        fread(chunkID, sizeof(char), 4, fp);
        if (strncmp(chunkID, "WAVE", 4) != 0)
        {
            fclose(fp);
            throw std::runtime_error("Le fichier n'est pas au format WAV");
        }

        // Search for the "fmt" chunk
        while (true)
        {
            fread(chunkID, sizeof(char), 4, fp);
            fread(&chunkSize, sizeof(uint32_t), 1, fp);

            if (strncmp(chunkID, "fmt ", 4) == 0) {
                fread(&format, sizeof(uint16_t), 1, fp);
                fread(&canaux, sizeof(uint16_t), 1, fp);
                fread(&taux_echantillonnage, sizeof(uint32_t), 1, fp);
                fseek(fp, 6, SEEK_CUR); // Ignore ByteRate and BlockAlign
                fread(&bits_par_echantillon, sizeof(uint16_t), 1, fp);
                break;
            }
            else
                fseek(fp, chunkSize, SEEK_CUR);
        }

        // Search for the "data" chunk
        while (true)
        {
            fread(chunkID, sizeof(char), 4, fp);
            fread(&chunkSize, sizeof(uint32_t), 1, fp);

            if (strncmp(chunkID, "data", 4) == 0)
            {
                dataSize = chunkSize;
                break;
            }
            else
                fseek(fp, chunkSize, SEEK_CUR);
        }

        if (dataSize == 0)
        {
            fclose(fp);
            throw std::runtime_error("Aucune donnée audio trouvée");
        }

        vector<ALchar> donnees(dataSize);
        fread(donnees.data(), sizeof(ALchar), dataSize, fp);
        fclose(fp);

        ALenum format_al;
        if (canaux == 1 && bits_par_echantillon == 8)
            format_al = AL_FORMAT_MONO8;
        else if (canaux == 1 && bits_par_echantillon == 16)
            format_al = AL_FORMAT_MONO16;
        else if (canaux == 2 && bits_par_echantillon == 8)
            format_al = AL_FORMAT_STEREO8;
        else if (canaux == 2 && bits_par_echantillon == 16)
            format_al = AL_FORMAT_STEREO16;
        else {
            throw std::runtime_error("Format audio non supporté");
        }

        alGenBuffers(1, &buffer);
        alBufferData(buffer, format_al, donnees.data(), dataSize, taux_echantillonnage);

        return (alGetError() == AL_NO_ERROR);
    }
};

class Sound 
{
private:
    ALuint source;
    ALuint buffer;

public:
    Sound(const string& fichier) 
    {
        SoundManager* manager = SoundManager::getInstance();
        buffer = manager->loadSound(fichier);
        if (buffer == 0) 
            throw std::runtime_error("Impossible de charger le fichier audio: " + fichier);

        alGenSources(1, &source);
        alSourcei(source, AL_BUFFER, buffer);
    }
    ~Sound() 
    {
        alDeleteSources(1, &source);
    }

    void play() {
        alSourcePlay(source);
    }
    void stop() {
        alSourceStop(source);
    }
    void pause() {
        // To pause the sound
        alSourcePause(source);
    }
    void setVolume(float volume) {
        alSourcef(source, AL_GAIN, volume);
    }
    void setPosition(float x, float y, float z) {
        alSource3f(source, AL_POSITION, x, y, z);
    }
    void setPosition(vec3 pos) {
        alSource3f(source, AL_POSITION, pos.x, pos.y, pos.z);
    }
    void setPitch(float pitch) {
        alSourcef(source, AL_PITCH, pitch);
    }
    void setLooping(bool loop) {
        alSourcei(source, AL_LOOPING, loop);
    }
    void adjustDistances() {
        alSourcef(source, AL_ROLLOFF_FACTOR, 1.0f);         // Rolloff factor
        alSourcef(source, AL_REFERENCE_DISTANCE, 50.0f);    // The sound starts to decrease after x units
        alSourcef(source, AL_MAX_DISTANCE, 5000.0f);        // The sound no longer drops after x units 
    }
};


