#include "mock_data.h"
#include <stddef.h>

// La biblioteca local de canciones hardcodeadas ha sido eliminada.
// MangoSpot ahora es únicamente un cliente Spotify Connect; la UI principal
// muestra una pantalla de bienvenida en lugar de la lista de álbumes mock.
// Se mantiene el array vacío para que el código legacy que aún referencia
// estos tipos compile mientras se reestructura.
Album library[] = {};
int library_count = 0;