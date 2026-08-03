# Versionado de MangoSpot

Este documento define el esquema de versiones y etiquetas para MangoSpot.

## Esquema: SemVer

Usamos [Semantic Versioning 2.0.0](https://semver.org/lang/es/):

```
MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]
```

- **MAJOR**: cambios incompatibles que rompen la experiencia anterior (ej. requiere reautenticación, cambia formato de credenciales guardadas).
- **MINOR**: nuevas funcionalidades retrocompatibles (ej. nueva pantalla, modo offline, soporte de listas).
- **PATCH**: correcciones de bugs o mejoras menores retrocompatibles (ej. fix de conexión, optimización de arranque).
- **PRERELEASE**: `alpha`, `beta`, `rc` seguido de número (ej. `beta.1`).
- **BUILD**: opcional, metadato de compilación (ej. `+switch`, `+devkitA64`).

## Etapas del proyecto

Dado que MangoSpot está en desarrollo activo y es homebrew no firmado, mantenemos el prefijo `0.x` hasta que se considere estable para uso diario.

- **`0.x.y-alpha`**: builds inestables, funcionalidad incompleta, posibles crashes.
- **`0.x.y-beta`**: funcionalidad casi completa, se buscan testers y reportes de bugs.
- **`0.x.y-rc`**: candidato a release; solo se aceptan correcciones críticas.
- **`1.0.0`**: primera versión estable pública.

## Versiones actuales

| Versión | Fecha | Descripción |
|---------|-------|-------------|
| `0.1.0-beta.1` | 2026-08-03 | Primera beta funcional: conexión Spotify arreglada, arranque optimizado, reproducción estable. |

## Cómo asignar una nueva versión

1. Decidir si el cambio es `MAJOR`, `MINOR` o `PATCH` según SemVer.
2. Actualizar `VERSIONING.md` añadiendo una fila en la tabla de versiones.
3. Actualizar `CHANGELOG.md` con los cambios de la versión.
4. Crear un tag de Git:

```bash
git tag -a v0.1.0-beta.1 -m "MangoSpot 0.1.0-beta.1: primera beta funcional"
git push origin main --tags
```

## Ejemplos futuros

- Fix de crash menor tras beta.1: `0.1.1-beta.2`
- Nueva pantalla de ajustes: `0.2.0-beta.1`
- Primera estable: `1.0.0`
