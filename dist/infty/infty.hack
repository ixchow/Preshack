frame 0
	top_color ( 0.4, 0.4, 0.4 )
	bottom_color ( 0.1, 0.1, 0.1 )
	area ( -1.3645, -2.79777 ) ( 6.97273, 1.06822 )
	screen_height 2
	screen_center ( -0.0311667, 0.0523185 )
	preview ( -1.10613, -0.38457 ) ( -0.181124, 0.489207 )
	nest 10 in 0
	nest 13 in 0
	nest 37 in 0
	nest 41 in 0
	nest 42 in 0
	nest 38 in 0
	nest 39 in 0
	nest 40 in 0
	nest 45 in 0
	nest 49 in 0
	nest 47 in 0
	nest 44 in 0
	nest 48 in 0
	nest 43 in 0
	nest 46 in 0
	nest 50 in 0
	nest 51 in 0
	nest 52 in 0
	nest 60 in 0
	nest 59 in 0
	nest 58 in 0
	nest 53 in 0
	nest 56 in 0
	nest 54 in 0
	nest 57 in 0
	nest 55 in 0
	connect 46 0 to 45 3 start ( -1.01638, -0.0382771 ) end ( 0.174653, -1.01676 )
	connect 46 0 to 43 3 end ( -0.564153, -1.01609 )
	connect 50 0 to 43 2 start ( 0.991365, -0.00867351 ) end ( -0.826426, -1.00505 )
	connect 50 0 to 45 2 start ( -1.01501, 0.0212773 ) end ( 0.380036, -0.99749 )
	connect 46 0 to 51 3 end ( -0.988326, -0.935725 )
	connect 50 0 to 51 2 end ( -0.999607, -0.817603 )
	position ( 0.0174154, -1.10009 )
	scale 0.640993
string 1 Infinite detail in 2d
	color ( 1, 1, 1 )
	position ( 0.0321202, -0.720817 )
	scale 0.1
frame 2
	top_color ( 0.4, 0.4, 0.4 )
	bottom_color ( 0.1, 0.1, 0.1 )
	area ( -1.38993, -2.45037 ) ( 5.8881, 0.996108 )
	screen_height 2
	screen_center ( -0.0565966, -0.003892 )
	preview ( -0.645691, 0.0367506 ) ( 0.04632, 0.707557 )
	nest 4 in 2
	nest 8 in 2
	nest 3 in 2
	nest 6 in 2
	nest 14 in 2
	nest 15 in 2
	nest 17 in 2
	nest 16 in 2
	nest 18 in 2
	nest 19 in 2
	nest 27 in 2
	nest 25 in 2
	nest 30 in 2
	nest 23 in 2
	nest 29 in 2
	nest 24 in 2
	nest 21 in 2
	nest 26 in 2
	nest 20 in 2
	nest 28 in 2
	nest 22 in 2
	nest 32 in 2
	nest 31 in 2
	nest 33 in 2
	nest 34 in 2
	nest 35 in 2
	connect 15 0 to 14 2
	connect 34 0 to 14 0 start ( 0.105902, 0.1365 )
	connect 35 0 to 14 1 start ( 0.360566, 0.16455 )
	position ( -1.07003, -1.09577 )
	scale 0.826598
equation 3 \left[\begin{array}{c}y\\z\\1\end{array}\right]
	position ( 0.156079, -0.712004 )
	scale 0.804785
string 4 The infinite projection matrix.
	color ( 1, 1, 1 )
	position ( 4.50327, 0.82482 )
	scale 0.138907
equation 5 \infty
	position ( -0.00961091, 0.543603 )
	scale 4.14147
equation 6 =
	position ( 0.391365, -0.708657 )
	scale 1
string 7 Infinite detail in 3d
	color ( 1, 1, 1 )
	position ( 1.07573, -0.718467 )
	scale 0.1
string 8 (mentioned by Jim Blinn around 1993)
	color ( 1, 1, 1 )
	position ( 4.58129, 0.698448 )
	scale 0.0871686
string 9 (in graphics)
	color ( 1, 1, 1 )
	position ( -0.0542615, 0.306077 )
	scale 0.1
string 10 What we're dealing with here is an interesting sort of tiling.
	color ( 1, 1, 1 )
	position ( 0.0296174, -0.865717 )
	scale 0.1
frame 11
	top_color ( 0.4, 0.4, 0.4 )
	bottom_color ( 0.1, 0.1, 0.1 )
	area ( -1.84095, -1.90757 ) ( 2.02429, 1.01432 )
	screen_height 2
	screen_center ( -0.04944, 0.01432 )
	preview ( -0.334712, 0.374869 ) ( 0.235832, 0.698894 )
	nest 5 in 11
	nest 9 in 11
	nest 12 in 11
	nest 2 in 11
	nest 1 in 11
	nest 7 in 11
	nest 0 in 11
	nest 36 in 11
	nest 64 in 11
	position ( 0, 0 )
	scale 0.5
string 12 Can we really see infinity?
	color ( 1, 1, 1 )
	position ( -1.11827, -0.725425 )
	scale 0.1
module 13 conformal infty/grid.png
	position ( 0.645132, -0.00435995 )
	scale 0.523131
module 14 projection 
	position ( 0.0424675, 0.378825 )
	scale 0.35443
module 15 slider 0 60 0 60
	position ( -1.03668, 0.149953 )
	scale 0.25
string 16 far
	color ( 1, 1, 1 )
	position ( -0.696863, -0.264394 )
	scale 0.1
equation 17 \theta
	position ( -1.03297, 0.261391 )
	scale 0.907409
string 18 near
	color ( 1, 1, 1 )
	position ( 0.0537238, -0.253379 )
	scale 0.1
equation 19 \left[\begin{array}{ccc}f&0&0\\0&a&b\\0&-1&0\end{array}\right]
	position ( -0.427965, -0.705477 )
	scale 0.85248
equation 20 b=\frac{2\cdot near \cdot far}{near - far}
	position ( 2.54898, 0.0922309 )
	scale 0.510423
equation 21 -a-\frac{b}{near} = -1
	position ( 1.60081, 0.414268 )
	scale 0.635028
string 22 Setting the z range.
	color ( 1, 1, 1 )
	position ( 2.08142, 0.815245 )
	scale 0.117079
string 23 Projection.
	color ( 1, 1, 1 )
	position ( -0.0996447, 0.815597 )
	scale 0.1
equation 24 -a-\frac{b}{far}=1
	position ( 1.57759, 0.0984245 )
	scale 0.668885
equation 25 \cong
	position ( 1.09864, -0.707894 )
	scale 1
equation 26 a = \frac{near+far}{near-far}
	position ( 2.54274, 0.407545 )
	scale 0.586814
equation 27 \left[\begin{array}{c}fy\\az+b\\-z\end{array}\right]
	position ( 0.732866, -0.720778 )
	scale 0.807503
equation 28 \implies
	position ( 2.12411, 0.261937 )
	scale 1
equation 29 f=\cot\frac{\theta}{2}
	position ( -1.0886, -1.08069 )
	scale 0.677957
equation 30 \left[\begin{array}{c}f\frac{y}{-z}\\-a-\frac{b}{z}\\1\end{array}\right]
	position ( 1.45038, -0.727807 )
	scale 0.775333
equation 31 \lim_{\alpha\to\infty}
	position ( 3.01429, -1.60091 )
	scale 1
equation 32 \left[\begin{array}{ccc}f&0&0\\0&-1&-2\cdot near\\0&-1&0\end{array}\right]
	position ( 3.76976, -1.63069 )
	scale 0.798703
equation 33 \left[\begin{array}{c}\alpha y \\ \alpha z \\ 1 \end{array}\right]
	position ( 4.52357, -1.6296 )
	scale 0.774467
module 34 slider 0 10 0 0
	position ( -0.0460624, -0.173956 )
	scale 0.25
module 35 slider 0 11 0 11
	position ( -0.674503, -0.163797 )
	scale 0.25
module 36 plane 
	position ( -0.0699511, -0.150062 )
	scale 0.338949
image 37 infty/droste.png
	position ( -0.632473, 0.0518619 )
	scale 1.48465
equation 38 \rightarrow
	position ( 1.39387, 0.306308 )
	scale 1.62714
equation 39 \leftarrow
	position ( 1.39137, -0.325923 )
	scale 1.53921
equation 40 \exp(z)
	position ( 1.39449, -0.432755 )
	scale 0.624021
module 41 conformal infty/grid.png 0 2 1
	position ( 2.08594, 0.00700278 )
	scale 0.504092
equation 42 \log(z)
	position ( 1.38359, 0.403297 )
	scale 0.787854
module 43 conformal infty/grid.png 0 2
	position ( 4.70819, -0.0213048 )
	scale 0.466016
equation 44 \rightarrow
	position ( 4.09393, -0.0409887 )
	scale 1.57902
module 45 conformal infty/grid.png 0 2 1
	position ( 3.44057, 0.00597298 )
	scale 0.478718
module 46 slider 0 10 1 0
	position ( 4.00509, -0.749017 )
	scale 0.25
equation 47 \times \alpha
	position ( 2.76812, 0.116822 )
	scale 1
equation 48 \exp(z)
	position ( 4.09716, 0.0664267 )
	scale 0.669438
equation 49 \leftrightarrow
	position ( 2.78453, 0.00813908 )
	scale 1.75103
module 50 slider 0 10 1 1
	position ( 3.9655, -0.622221 )
	scale 0.25
module 51 conformal infty/ronit.png 0 4
	position ( 6.30104, -0.101621 )
	scale 0.644539
image 52 infty/print_gallery.png
	position ( -0.00583114, -1.85901 )
	scale 1.74699
string 53 echerdroste.math.leidenuniv.nl
	color ( 1, 1, 1 )
	position ( -0.0223027, 0.936588 )
	scale 0.104226
equation 54 \log(r e ^{i\theta})\equiv\ln(r) + i\theta
	position ( 1.3822, 0.679164 )
	scale 1
equation 55 f(\exp(\log(z)) = f(\exp(\log(s) + \log(z)))
	position ( 1.99879, -0.682699 )
	scale 0.695584
image 56 infty/leiden_droste.png
	position ( 5.44019, -1.8747 )
	scale 0.954704
equation 57 f(z)=f(s\cdot z)
	position ( 0.626576, -0.668468 )
	scale 1
image 58 infty/escher_droste.png
	position ( 2.47606, -1.50651 )
	scale 0.857383
image 59 infty/leiden.png
	position ( 6.43364, -1.87455 )
	scale 0.947134
image 60 infty/escher_grid.png
	position ( 4.01688, -1.79231 )
	scale 1.32585
image 61 infty/frac_solids.png
	position ( -0.624539, -0.280677 )
	scale 0.957234
image 62 infty/frac_trees.png
	position ( 0.611787, -0.283564 )
	scale 0.963133
image 63 infty/frac_title.png
	position ( -0.0697046, 0.624201 )
	scale 0.553397
frame 64
	top_color ( 0.4, 0.4, 0.4 )
	bottom_color ( 0.1, 0.1, 0.1 )
	area ( -1.41003, -6.75767 ) ( 3.98683, 1.0043 )
	screen_height 2
	screen_center ( -0.0766966, -0.262175 )
	preview ( -0.947634, -0.573226 ) ( -0.325252, 0.0488763 )
	nest 63 in 64
	nest 62 in 64
	nest 61 in 64
	nest 70 in 64
	nest 66 in 64
	nest 71 in 64
	nest 68 in 64
	nest 69 in 64
	nest 65 in 64
	nest 67 in 64
	nest 73 in 64
	nest 72 in 64
	position ( 1.07824, -1.09913 )
	scale 0.90497
image 65 infty/frac_filter.png
	position ( -0.702225, -4.33501 )
	scale 0.328589
image 66 infty/frac_bounding.png
	position ( -0.688505, -1.79113 )
	scale 0.863449
image 67 infty/frac_computer.png
	position ( 2.57537, 0.656202 )
	scale 0.345489
string 68 3) Antialiasing.
	color ( 1, 1, 1 )
	position ( -1.00222, -4.84647 )
	scale 0.1
image 69 infty/frac_extrude.png
	position ( -0.699869, -3.63958 )
	scale 0.855471
string 70 1) Tracing Rays:
	color ( 1, 1, 1 )
	position ( -0.947978, -1.06508 )
	scale 0.1
string 71 2) Computing Normals:
	color ( 1, 1, 1 )
	position ( -0.888769, -3.02398 )
	scale 0.1
string 72 (definition of geometry)
	color ( 1, 1, 1 )
	position ( 2.58025, 0.249856 )
	scale 0.1
equation 73 \mathbf{w}(s)=\bigcup_{i=1}^Nw_i(s)
	position ( 2.11842, -0.130413 )
	scale 1
stack 11
