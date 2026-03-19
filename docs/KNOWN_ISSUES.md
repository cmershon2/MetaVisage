# Known Issues - MetaVisage v1.0.0

## Minor Issues

### Rendering

- **No OpenGL error reporting**: OpenGL operations do not check `glGetError()` after critical calls. Silent failures may occur on some GPU configurations.

### File I/O

- **No try-catch for file operations**: File I/O operations do not use exception handling. Corrupt or inaccessible files may cause unexpected behavior rather than graceful error messages.

### UI

- **Layout rebuild uses manual deletion**: The sidebar widget clears its layout using manual `delete` calls rather than `deleteLater()`. This is functionally correct but not ideal Qt practice.

### Deformation

- **RBF solver edge case**: For very small control point sets (< 4 points) with zero stiffness, the augmented linear system may become under-determined. The solver will add minimal regularization automatically, but results may be less precise.

## Platform Limitations

- **Windows only**: Currently only Windows 10/11 (64-bit) is supported. macOS and Linux support is planned for a future release.
- **No auto-save**: Project auto-save is not implemented. Save frequently using Ctrl+S.
- **No crash recovery**: If the application crashes, unsaved work is lost.

## Unreal Engine Compatibility Notes

- Tested with Unreal Engine 5.x
- Exported meshes use Y-up coordinate system
- Material slot names are preserved from the original MetaHuman mesh
- UV seam vertices are properly handled during export via vertex splitting

## Reporting New Issues

Please report issues on the [GitHub Issues page](https://github.com/cmershon2/MetaVisage/issues) with:

- OS version and GPU model
- Steps to reproduce
- Mesh details (format, vertex count)
- Log file contents (if available)
