Extensive testing has been done and changes have been requested
## General
1. An orientation gizmo in the top right corner would be helpful as well to see which way is X/Y/Z
2. landing pop-up projects are not scrollable and squish together
3. 3d grid is not infinite
4. lighting is static and does not move with the camera
5. FPS counter is not always accurate - we should remove this
6. When a user's system uses Light mode, it changes the background of the application to be light (except behind the settings). The background of the application should always be dark.
7. There is no installer for the application - we should use Inno Setup for installation
8. There is no way to handle updates in the application - we should use WinSparkle for this
## Stage 1
1. Shading Mode selector does not work in stage one
2. Shading mode should be a setting at the top left of the 3D port that is visible in all stages for the user to adjust
3. Need gizmos on stage 1 for rotation, scaling, and positioning
4. Rotation in stage 1 does not align between the view port and side panel
5. Textures are limited to just 2 (Albedo & Normal Map), the user should be able to upload as many as desired. 
6. Textures should have a small preview/thumbnail of what they look like beside the file name. Clicking the preview should open a modal where the user can see the texture.
## Stage 2
1.  If a project is saved and opened at a later date on Stage 2, the points still exist, however the `Correspondence Points` field is empty, the target & morph points show zero, and enable symmetry mode shows disabled (Even when it was previously enabled).

## Stage 3
1. We should remove all RBF algorithms - these are no longer relavent as NRICP outperforms them.
2. We should remove the mention of NRICP to protect the algorithm from being exposed
3. We should add presets for Quick Wrap & Fantasy Wrap. Fantasy wrap should use the current presets defined and is for characters with more dramatic facial features such as pointed ears or large noes. While Quick Wrap will be for more human looking characters. The only difference will be that Quick Wrap should set rigidity parameters to 1.
4. Users should be able to save their own wrapping presets.
5. Wrapping presets should be displayed as small cards at the top with the parameters in collapsible sections underneath. The preset options should have a graphic thumbnail
6. Parameters should be moved to collapsible sections to save space in the sidebar. Below is the current idea:
	1. Vertex Masking - this should not be a collapsible section as it can pertain to any of the presets.
	2. General
		1. Stiffness Init
		2. Stiffness Final
		3. Stiffness Steps
		4. ICP Iterations
		5. Normal Thresh
		6. Landmark Wt
		7. Interior Geometry
			1. Exclude Boundary Regions
			2. Boundry Depth
	3. Advanced
		1. Optimization
			1. Opt Iterations
			2. Dp Initial
			3. Dp Final
		2. Rigidity
			1. Rigidity Init
			2. Rigidity Final
		3. Control Node Sampling
			1. Sampling Init
			2. Sampling Final
			3. Normalize Sampling
7. Not sure if this is possible to fix but user's have been reporting "cottage cheese face" after morphing. It seems that where points are on the face, seem to drastically stretch the geometry sometimes.
## Stage 4
1. Make brush selector more visual - add sample images of what each brush does
## Top nav bar
1. Help > Documentation link does not open documentation site (https://docs.metavisage.io)
2. Tools section is only relevant on stage 1 - this should be removed 
3. View settings do not do anything
4. Edit > Preferences does not do anything and can probably be removed
5. File > Export Mesh is only relevant on stage 4 - this should be removed