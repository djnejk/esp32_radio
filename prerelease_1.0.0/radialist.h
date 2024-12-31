#ifndef RADIOLIST_H
#define RADIOLIST_H

struct Radio {
    const char* slug;
    const char* name;
};

Radio radialist[] = {
    {"cesky-rozhlas-radiozurnal", "Český rozhlas Radiožurnál"},
    {"cesky-rozhlas-dvojka", "Český rozhlas Dvojka"},
    {"cesky-rozhlas-plus", "Český rozhlas Plus"},
    {"cesky-rozhlas-vltava", "Český rozhlas Vltava"},
    {"cesky-rozhlas-jazz", "Český rozhlas Jazz"},
    {"cesky-rozhlas-radio-wave", "Český rozhlas Radio Wave"},
    {"cesky-rozhlas-d-dur", "Český rozhlas D-dur"},
    {"cesky-rozhlas-plzen", "Český rozhlas Plzeň"},
    {"cesky-rozhlas-ostrava", "Český rozhlas Ostrava"},
    {"cesky-rozhlas-brno", "Český rozhlas Brno"}
    // Přidejte další rádia dle potřeby
};

const int radialist_size = sizeof(radialist) / sizeof(radialist[0]);

#endif // RADIOLIST_H
