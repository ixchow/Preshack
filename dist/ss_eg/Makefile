all : \
alpha_analogy.png \
alpha_anim.s1.png \
alpha_anim.s2.png \
alpha_anim.s3.png \
alpha_anim.s4.png \
alpha_anim.s5.png \
swap.opaque.png \
swap.transp.png \
swap.one.png \
swap.both.png \
armies.png \

alpha_anim.%.svg : alpha_anim.svg
	./svgsplit.py $< $* > $@

swap.%.svg : swap.svg
	./svgsplit.py $< $* > $@

%.png : %.svg
	inkscape --export-png=$@ --export-dpi=200 --export-area-drawing --export-background='#000000' --export-background-opacity=0.0 $<
