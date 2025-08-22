/* SimShip by Edouard Halbert
This work is licensed under a Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License
http://creativecommons.org/licenses/by-nc-nd/4.0/ */

#pragma once

#include <cmath>
#include <vector>
#include <algorithm>

// Mer en développement dépendant du fetch
class JONSWAPModel 
{
private:
    static constexpr double g = 9.81; // Accélération due à la gravité (m/s^2)

public:
    struct WaveParameters 
    {
        double significantWaveHeight; // Hs en mètres
        double peakPeriod;            // Tp en secondes
    };

    static WaveParameters GetWaveParameters(double windSpeed, double fetch) 
    {
        // Conversion de la vitesse du vent à 10m de hauteur
        double U10 = windSpeed;

        // Calcul des paramètres sans dimension
        double gF = g * fetch / (U10 * U10);
        double gT = g * peakPeriod(U10, fetch) / U10;

        // Calcul de la hauteur significative des vagues
        double Hs = 0.0016 * sqrt(gF) * U10 * U10 / g;

        // Calcul de la période de pic
        double Tp = peakPeriod(U10, fetch);

        return { Hs, Tp };
    }

    static double peakPeriod(double U10, double fetch) 
    {
        return 0.33 * pow(g * fetch / (U10 * U10), 0.33) * U10 / g;
    }
};

// Le spectre de Phillips modélise l'énergie des vagues générées par le vent avec une forme théorique, souvent exprimée en termes de nombre d'onde k ou fréquence f.
// La constante de Phillips alpha(Ph) vaut classiquement environ 0.0081.
class PhillipsModel
{
private:
    static constexpr double g = 9.81;  // Accélération gravitationnelle m/s^2

public:
    struct WaveParameters
    {
        double significantWaveHeight; // Hs en mètres
        double peakPeriod;            // Tp en secondes
    };

    // Calcule Hs et Tp à partir du vent U10 (en m/s) selon formules simplifiées du spectre de Phillips
    static WaveParameters GetWaveParameters(double windSpeed)
    {
        double U10 = windSpeed;

        // Coefficients empiriques typiques pour Phillips (à ajuster selon besoins)
        constexpr double C_H = 0.2;   // Coefficient pour Hs
        constexpr double C_T = 1.5;   // Coefficient pour Tp

        // Calcul hauteur significative (m)
        double Hs = C_H * (U10 * U10) / g;

        // Calcul période de pic (s)
        double Tp = C_T * U10 / g;

        return { Hs, Tp };
    }
};

// Mer pleinement développée
class PiersonMoskowitzModel
{
private:
    static constexpr double g = 9.81;

public:
    struct WaveParameters
    {
        double significantWaveHeight; // Hs en m
        double peakPeriod;            // Tp en s
    };

    static WaveParameters GetWaveParameters(double windSpeed) // windSpeed en m/s
    {
        double U10 = windSpeed;

        double Tp = 0.13 * U10;             // période de pic Tp (en s)
        double Hs = 0.24 * U10 * U10 / g;  // hauteur significative Hs (en m)

        return { Hs, Tp };
    }
};

// Spectre utilisé en eau peu profonde
class TMA_Model
{
private:
    static constexpr double g = 9.81;

public:
    struct WaveParameters
    {
        double significantWaveHeight;
        double peakPeriod;
    };

    static WaveParameters GetWaveParameters(double windSpeed)
    {
        double U10 = windSpeed;

        // Approximations simples (exemple)
        double Tp = 0.12 * U10;  // période de pic Tp (s)
        double Hs = 0.20 * U10 * U10 / g; // hauteur significative (m)

        return { Hs, Tp };
    }
};

// Modèle initial Pierson-Moskowitz + paramétrisation de alpha
class HasselmannModel
{
private:
    static constexpr double g = 9.81;

public:
    struct WaveParameters
    {
        double significantWaveHeight;
        double peakPeriod;
    };

    static WaveParameters GetWaveParameters(double windSpeed)
    {
        double U10 = windSpeed;

        double Tp = 0.14 * U10;
        double Hs = 0.21 * U10 * U10 / g;

        return { Hs, Tp };
    }
};

// Le modèle de Donelan et Banner [1985,[1996] propose une description spectrale basée sur deux régimes, avec un ajustement pour la pente du spectre, complexe à calculer intégralement. 
// La hauteur significative et la période de pic peuvent être approchées par des formules empiriques basées sur U10 et fetch.
class DonelanBannerModel
{
private:
    static constexpr double g = 9.81;

public:
    struct WaveParameters
    {
        double significantWaveHeight; // Hs en m
        double peakPeriod;            // Tp en s
    };

    static WaveParameters GetWaveParameters(double windSpeed, double fetch)
    {
        double U10 = windSpeed;
        // Formule empirique adaptée de Donelan & Banner (simplification)
        double gF = g * fetch / (U10 * U10);

        // Hauteur significative approximative
        double Hs = 0.0012 * std::sqrt(gF) * U10 * U10 / g;

        // Période de pic (exemple simplifié)
        double Tp = 0.3 * std::pow(gF, 0.25) * U10 / g;

        return { Hs, Tp };
    }
};

// Le spectre Elfouhaily est un modèle unifié directionnel très utilisé, qui décrit à la fois les vagues de gravité et capillaires.
class ElfouhailyModel
{
private:
    static constexpr double g = 9.81;

public:
    struct WaveParameters
    {
        double significantWaveHeight;
        double peakPeriod;
    };

    static WaveParameters GetWaveParameters(double windSpeed, double fetch)
    {
        double U10 = windSpeed;
        double gF = g * fetch / (U10 * U10);

        // Hauteur significative estimée (approximation adaptée)
        double Hs = 0.0014 * std::sqrt(gF) * U10 * U10 / g;

        // Période de pic selon Elfouhaily (simplifiée)
        double Tp = 0.32 * std::pow(gF, 0.33) * U10 / g;

        return { Hs, Tp };
    }
};
void DisplayWaveParametersFromModels(double windSpeed, double fetch)
{
    cout << "Significant wave heights (1/3)" << endl;
    cout << "==============================" << endl;
    auto waveParams0 = JONSWAPModel::GetWaveParameters(windSpeed, fetch);
    cout << fixed << setprecision(2);
    cout << "JONSWAP : \t\t" << waveParams0.significantWaveHeight << " m - Period : " << waveParams0.peakPeriod << " s" << endl;

    auto waveParams00 = PhillipsModel::GetWaveParameters(windSpeed);
    cout << "Phillips : \t\t" << waveParams00.significantWaveHeight << " m - Period : " << waveParams00.peakPeriod << " s" << endl;

    auto waveParams1 = PiersonMoskowitzModel::GetWaveParameters(windSpeed);
    cout << "PiersonMoskowitz : \t" << waveParams1.significantWaveHeight << " m - Period : " << waveParams1.peakPeriod << " s" << endl;

    auto waveParams2 = TMA_Model::GetWaveParameters(windSpeed);
    cout << "TMA : \t\t\t" << waveParams2.significantWaveHeight << " m - Period : " << waveParams2.peakPeriod << " s" << endl;

    auto waveParams3 = HasselmannModel::GetWaveParameters(windSpeed);
    cout << "Hasselmann : \t\t" << waveParams3.significantWaveHeight << " m - Period : " << waveParams3.peakPeriod << " s" << endl;

    auto waveParams4 = DonelanBannerModel::GetWaveParameters(windSpeed, fetch);
    cout << "DonelanBanner : \t" << waveParams4.significantWaveHeight << " m - Period : " << waveParams4.peakPeriod << " s" << endl;

    auto waveParams5 = ElfouhailyModel::GetWaveParameters(windSpeed, fetch);
    cout << "Elfouhaily : \t\t" << waveParams5.significantWaveHeight << " m - Period : " << waveParams5.peakPeriod << " s" << endl;

    cout << "==============================" << endl;
    cout << endl;
}
