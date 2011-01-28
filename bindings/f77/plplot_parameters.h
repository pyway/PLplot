      integer*4 PLESC_SET_RGB ! obsolete
      parameter (PLESC_SET_RGB = 1)
      integer*4 PLESC_ALLOC_NCOL ! obsolete
      parameter (PLESC_ALLOC_NCOL = 2)
      integer*4 PLESC_SET_LPB ! obsolete
      parameter (PLESC_SET_LPB = 3)
      integer*4 PLESC_EXPOSE ! handle window expose
      parameter (PLESC_EXPOSE = 4)
      integer*4 PLESC_RESIZE ! handle window resize
      parameter (PLESC_RESIZE = 5)
      integer*4 PLESC_REDRAW ! handle window redraw
      parameter (PLESC_REDRAW = 6)
      integer*4 PLESC_TEXT ! switch to text screen
      parameter (PLESC_TEXT = 7)
      integer*4 PLESC_GRAPH ! switch to graphics screen
      parameter (PLESC_GRAPH = 8)
      integer*4 PLESC_FILL ! fill polygon
      parameter (PLESC_FILL = 9)
      integer*4 PLESC_DI ! handle DI command
      parameter (PLESC_DI = 10)
      integer*4 PLESC_FLUSH ! flush output
      parameter (PLESC_FLUSH = 11)
      integer*4 PLESC_EH ! handle Window events
      parameter (PLESC_EH = 12)
      integer*4 PLESC_GETC ! get cursor position
      parameter (PLESC_GETC = 13)
      integer*4 PLESC_SWIN ! set window parameters
      parameter (PLESC_SWIN = 14)
      integer*4 PLESC_PLFLTBUFFERING ! configure PLFLT buffering
      parameter (PLESC_PLFLTBUFFERING = 15)
      integer*4 PLESC_XORMOD ! set xor mode
      parameter (PLESC_XORMOD = 16)
      integer*4 PLESC_SET_COMPRESSION ! AFR: set compression
      parameter (PLESC_SET_COMPRESSION = 17)
      integer*4 PLESC_CLEAR ! RL: clear graphics region
      parameter (PLESC_CLEAR = 18)
      integer*4 PLESC_DASH ! RL: draw dashed line
      parameter (PLESC_DASH = 19)
      integer*4 PLESC_HAS_TEXT ! driver draws text
      parameter (PLESC_HAS_TEXT = 20)
      integer*4 PLESC_IMAGE ! handle image
      parameter (PLESC_IMAGE = 21)
      integer*4 PLESC_IMAGEOPS ! plimage related operations
      parameter (PLESC_IMAGEOPS = 22)
      integer*4 DRAW_LINEX ! draw lines parallel to the X axis
      parameter (DRAW_LINEX = z'01')
      integer*4 DRAW_LINEY ! draw lines parallel to the Y axis
      parameter (DRAW_LINEY = z'02')
      integer*4 DRAW_LINEXY ! draw lines parallel to both the X and Y axes
      parameter (DRAW_LINEXY = z'03')
      integer*4 MAG_COLOR ! draw the mesh with a color dependent of the magnitude
      parameter (MAG_COLOR = z'04')
      integer*4 BASE_CONT ! draw contour plot at bottom xy plane
      parameter (BASE_CONT = z'08')
      integer*4 TOP_CONT ! draw contour plot at top xy plane
      parameter (TOP_CONT = z'10')
      integer*4 SURF_CONT ! draw contour plot at surface
      parameter (SURF_CONT = z'20')
      integer*4 DRAW_SIDES ! draw sides
      parameter (DRAW_SIDES = z'40')
      integer*4 FACETED ! draw outline for each square that makes up the surface
      parameter (FACETED = z'80')
      integer*4 MESH ! draw mesh
      parameter (MESH = z'100')
      integer*4 PL_BIN_DEFAULT 
      parameter (PL_BIN_DEFAULT = 0)
      integer*4 PL_BIN_CENTRED 
      parameter (PL_BIN_CENTRED = 1)
      integer*4 PL_BIN_NOEXPAND 
      parameter (PL_BIN_NOEXPAND = 2)
      integer*4 PL_BIN_NOEMPTY 
      parameter (PL_BIN_NOEMPTY = 4)
      integer*4 PL_HIST_DEFAULT 
      parameter (PL_HIST_DEFAULT = 0)
      integer*4 PL_HIST_NOSCALING 
      parameter (PL_HIST_NOSCALING = 1)
      integer*4 PL_HIST_IGNORE_OUTLIERS 
      parameter (PL_HIST_IGNORE_OUTLIERS = 2)
      integer*4 PL_HIST_NOEXPAND 
      parameter (PL_HIST_NOEXPAND = 8)
      integer*4 PL_HIST_NOEMPTY 
      parameter (PL_HIST_NOEMPTY = 16)
      integer*4 PL_POSITION_LEFT 
      parameter (PL_POSITION_LEFT = 1)
      integer*4 PL_POSITION_RIGHT 
      parameter (PL_POSITION_RIGHT = 2)
      integer*4 PL_POSITION_UPPER 
      parameter (PL_POSITION_UPPER = 4)
      integer*4 PL_POSITION_LOWER 
      parameter (PL_POSITION_LOWER = 8)
      integer*4 PL_POSITION_INSIDE 
      parameter (PL_POSITION_INSIDE = 16)
      integer*4 PL_POSITION_OUTSIDE 
      parameter (PL_POSITION_OUTSIDE = 32)
      integer*4 PL_POSITION_VIEWPORT 
      parameter (PL_POSITION_VIEWPORT = 64)
      integer*4 PL_POSITION_SUBPAGE 
      parameter (PL_POSITION_SUBPAGE = 128)
      integer*4 PL_LEGEND_NONE 
      parameter (PL_LEGEND_NONE = 1)
      integer*4 PL_LEGEND_COLOR_BOX 
      parameter (PL_LEGEND_COLOR_BOX = 2)
      integer*4 PL_LEGEND_LINE 
      parameter (PL_LEGEND_LINE = 4)
      integer*4 PL_LEGEND_SYMBOL 
      parameter (PL_LEGEND_SYMBOL = 8)
      integer*4 PL_LEGEND_TEXT_LEFT 
      parameter (PL_LEGEND_TEXT_LEFT = 16)
      integer*4 PL_LEGEND_BACKGROUND 
      parameter (PL_LEGEND_BACKGROUND = 32)
      integer*4 PL_LEGEND_BOUNDING_BOX 
      parameter (PL_LEGEND_BOUNDING_BOX = 64)
      integer*4 PL_LEGEND_ROW_MAJOR 
      parameter (PL_LEGEND_ROW_MAJOR = 128)
      integer*4 PL_COLORBAR_LEFT 
      parameter (PL_COLORBAR_LEFT = 1)
      integer*4 PL_COLORBAR_RIGHT 
      parameter (PL_COLORBAR_RIGHT = 2)
      integer*4 PL_COLORBAR_UPPER 
      parameter (PL_COLORBAR_UPPER = 4)
      integer*4 PL_COLORBAR_LOWER 
      parameter (PL_COLORBAR_LOWER = 8)
      integer*4 PL_COLORBAR_LABEL_LEFT 
      parameter (PL_COLORBAR_LABEL_LEFT = 16)
      integer*4 PL_COLORBAR_LABEL_RIGHT 
      parameter (PL_COLORBAR_LABEL_RIGHT = 32)
      integer*4 PL_COLORBAR_LABEL_UPPER 
      parameter (PL_COLORBAR_LABEL_UPPER = 64)
      integer*4 PL_COLORBAR_LABEL_LOWER 
      parameter (PL_COLORBAR_LABEL_LOWER = 128)
      integer*4 PL_COLORBAR_IMAGE 
      parameter (PL_COLORBAR_IMAGE = 256)
      integer*4 PL_COLORBAR_SHADE 
      parameter (PL_COLORBAR_SHADE = 512)
      integer*4 PL_COLORBAR_GRADIENT 
      parameter (PL_COLORBAR_GRADIENT = 1024)
      integer*4 PL_COLORBAR_CAP_LOW 
      parameter (PL_COLORBAR_CAP_LOW = 2048)
      integer*4 PL_COLORBAR_CAP_HIGH 
      parameter (PL_COLORBAR_CAP_HIGH = 4096)
      integer*4 PL_COLORBAR_SHADE_LABEL 
      parameter (PL_COLORBAR_SHADE_LABEL = 8192)
      integer*4 PLSWIN_DEVICE ! device coordinates
      parameter (PLSWIN_DEVICE = 1)
      integer*4 PLSWIN_WORLD ! world coordinates
      parameter (PLSWIN_WORLD = 2)
      integer*4 PL_X_AXIS ! The x-axis
      parameter (PL_X_AXIS = 1)
      integer*4 PL_Y_AXIS ! The y-axis
      parameter (PL_Y_AXIS = 2)
      integer*4 PL_Z_AXIS ! The z-axis
      parameter (PL_Z_AXIS = 3)
      integer*4 PL_OPT_ENABLED ! Obsolete
      parameter (PL_OPT_ENABLED = z'0001')
      integer*4 PL_OPT_ARG ! Option has an argment
      parameter (PL_OPT_ARG = z'0002')
      integer*4 PL_OPT_NODELETE ! Don't delete after processing
      parameter (PL_OPT_NODELETE = z'0004')
      integer*4 PL_OPT_INVISIBLE ! Make invisible
      parameter (PL_OPT_INVISIBLE = z'0008')
      integer*4 PL_OPT_DISABLED ! Processing is disabled
      parameter (PL_OPT_DISABLED = z'0010')
      integer*4 PL_OPT_FUNC ! Call handler function
      parameter (PL_OPT_FUNC = z'0100')
      integer*4 PL_OPT_BOOL ! Set *var = 1
      parameter (PL_OPT_BOOL = z'0200')
      integer*4 PL_OPT_INT ! Set *var = atoi(optarg)
      parameter (PL_OPT_INT = z'0400')
      integer*4 PL_OPT_FLOAT ! Set *var = atof(optarg)
      parameter (PL_OPT_FLOAT = z'0800')
      integer*4 PL_OPT_STRING ! Set var = optarg
      parameter (PL_OPT_STRING = z'1000')
      integer*4 PL_PARSE_PARTIAL ! For backward compatibility
      parameter (PL_PARSE_PARTIAL = z'0000')
      integer*4 PL_PARSE_FULL ! Process fully & exit if error
      parameter (PL_PARSE_FULL = z'0001')
      integer*4 PL_PARSE_QUIET ! Don't issue messages
      parameter (PL_PARSE_QUIET = z'0002')
      integer*4 PL_PARSE_NODELETE ! Don't delete options after
      parameter (PL_PARSE_NODELETE = z'0004')
      integer*4 PL_PARSE_SHOWALL ! Show invisible options
      parameter (PL_PARSE_SHOWALL = z'0008')
      integer*4 PL_PARSE_OVERRIDE ! Obsolete
      parameter (PL_PARSE_OVERRIDE = z'0010')
      integer*4 PL_PARSE_NOPROGRAM ! Program name NOT in *argv[0]..
      parameter (PL_PARSE_NOPROGRAM = z'0020')
      integer*4 PL_PARSE_NODASH ! Set if leading dash NOT required
      parameter (PL_PARSE_NODASH = z'0040')
      integer*4 PL_PARSE_SKIP ! Skip over unrecognized args
      parameter (PL_PARSE_SKIP = z'0080')
      integer*4 PL_FCI_MARK 
      parameter (PL_FCI_MARK = ishft(1,31))
      integer*4 PL_FCI_IMPOSSIBLE 
      parameter (PL_FCI_IMPOSSIBLE = z'00000000')
      integer*4 PL_FCI_HEXDIGIT_MASK 
      parameter (PL_FCI_HEXDIGIT_MASK = z'f')
      integer*4 PL_FCI_HEXPOWER_MASK 
      parameter (PL_FCI_HEXPOWER_MASK = z'7')
      integer*4 PL_FCI_HEXPOWER_IMPOSSIBLE 
      parameter (PL_FCI_HEXPOWER_IMPOSSIBLE = z'f')
      integer*4 PL_FCI_FAMILY 
      parameter (PL_FCI_FAMILY = z'0')
      integer*4 PL_FCI_STYLE 
      parameter (PL_FCI_STYLE = z'1')
      integer*4 PL_FCI_WEIGHT 
      parameter (PL_FCI_WEIGHT = z'2')
      integer*4 PL_FCI_SANS 
      parameter (PL_FCI_SANS = z'0')
      integer*4 PL_FCI_SERIF 
      parameter (PL_FCI_SERIF = z'1')
      integer*4 PL_FCI_MONO 
      parameter (PL_FCI_MONO = z'2')
      integer*4 PL_FCI_SCRIPT 
      parameter (PL_FCI_SCRIPT = z'3')
      integer*4 PL_FCI_SYMBOL 
      parameter (PL_FCI_SYMBOL = z'4')
      integer*4 PL_FCI_UPRIGHT 
      parameter (PL_FCI_UPRIGHT = z'0')
      integer*4 PL_FCI_ITALIC 
      parameter (PL_FCI_ITALIC = z'1')
      integer*4 PL_FCI_OBLIQUE 
      parameter (PL_FCI_OBLIQUE = z'2')
      integer*4 PL_FCI_MEDIUM 
      parameter (PL_FCI_MEDIUM = z'0')
      integer*4 PL_FCI_BOLD 
      parameter (PL_FCI_BOLD = z'1')
      integer*4 PL_MAXKEY 
      parameter (PL_MAXKEY = 16)
      integer*4 PL_MAXWINDOWS ! Max number of windows/page tracked
      parameter (PL_MAXWINDOWS = 64)
      real*8 PL_NOTSET 
      parameter (PL_NOTSET = -42.0d0)
      real*8 PI
      parameter (PI = 3.1415926535897932384d0)
      real*8 TWOPI
      parameter (TWOPI = 2.0d0*PI)
      integer*4 PLESPLFLTBUFFERING_ENABLE 
      parameter (PLESPLFLTBUFFERING_ENABLE = 1)
      integer*4 PLESPLFLTBUFFERING_DISABLE 
      parameter (PLESPLFLTBUFFERING_DISABLE = 2)
      integer*4 PLESPLFLTBUFFERING_QUERY 
      parameter (PLESPLFLTBUFFERING_QUERY = 3)
      integer*4 GRID_CSA ! Bivariate Cubic Spline approximation
      parameter (GRID_CSA = 1)
      integer*4 GRID_DTLI ! Delaunay Triangulation Linear Interpolation
      parameter (GRID_DTLI = 2)
      integer*4 GRID_NNI ! Natural Neighbors Interpolation
      parameter (GRID_NNI = 3)
      integer*4 GRID_NNIDW ! Nearest Neighbors Inverse Distance Weighted
      parameter (GRID_NNIDW = 4)
      integer*4 GRID_NNLI ! Nearest Neighbors Linear Interpolation
      parameter (GRID_NNLI = 5)
      integer*4 GRID_NNAIDW ! Nearest Neighbors Around Inverse Distance Weighted
      parameter (GRID_NNAIDW = 6)
