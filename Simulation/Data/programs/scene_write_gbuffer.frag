 ފ         #     ?                GLSL.std.450              	       main    �   �                    �   
 GL_GOOGLE_cpp_style_line_directive       main     	    material_texture_descriptor          sampler_idx   	   material_descriptor   	       cavity_map    	      normal_map    	      mask_map      	      texture   	      emission     	 	      packed_emission_color     	      head_layer    	      material_flags   9    material_is_masked(struct-material_descriptor-struct-material_texture_descriptor-u11-struct-material_texture_descriptor-u11-struct-material_texture_descriptor-u11-struct-material_texture_descriptor-u11-f1-u1-u1-u11;vf2;      md       uv       _norm_oct_wrap(vf2;      v    	    norm3x32_to_snorm2x32(vf3;       n     "   g_buffer_element      "       data      +   gbuffer_encode(f1;vf2;vf2;vf2;vf3;vf3;i1;     $   depth     %   UV    &   dUVdx     '   dUVdy     (   N     )   T     *   material_id   8   material_textures_count   ;   material_textures     E   material_sampler         param     �   dUVdx16   �   dUVdy16   �   Npack     �   param     �   Tpack     �   param     �   g_frag    �   material      �   scene_transform   �       frag_normal   �      frag_tangent      �      frag_texcoords    �      material_id   �   vin   �   uv    �   md   	 �   material_texture_descriptor   �       sampler_idx   �   material_descriptor   �       cavity_map    �      normal_map    �      mask_map      �      texture   �      emission     	 �      packed_emission_color     �      head_layer    �      material_flags   
 �   material_descriptors_binding      �       mat_descriptor    �         �   param     �   param     �   n     �   t     �   gbuffer_element   �   gl_FragCoord         param       param       param       param       param     
  param       param       gbuffer0        gbuffer1     	 +  material_layer_descriptor    	 +      roughness_sampler_idx    	 +     metallicity_sampler_idx  	 +     thickness_sampler_idx     +     next_layer_id    	 +     attenuation_coefficient   +     packed_albedo     +     ior_phase_pack    +     _unused1      +     _unused2      -  material_layer_descriptors_binding   	 -      mat_layer_descriptor      /        :  gbuffer   <  depth_map     =  downsampled_depth_map     >  backface_depth_map  G  8          G  ;   "      G  ;   !      G  E   "      G  E   !      H  �         G  �      G  �          H  �       #       H  �       #       H  �      #      H  �      #      H  �      #      H  �      #      H  �      #      H  �      #      H  �      #      G  �          H  �          H  �          H  �       #       G  �      G  �   "      G  �   !      G  �         G           G          H  +      #       H  +     #      H  +     #      H  +     #      H  +     #      H  +     #       H  +     #   $   H  +     #   (   H  +     #   ,   G  ,     0   H  -         H  -         H  -      #       G  -     G  /  "      G  /  !      G  :  "      G  :  !      G  <  "      G  <  !      G  =  "      G  =  !      G  >  "      G  >  !           !                                       
 	                              
      	                               !        
      !                                  !                                                             !            "   !   ! 
 #   "                        +     -         .         +     1      +     3        	 7                            2     8        9   7   8      :       9   ;  :   ;       +     <      +     =          @       7     C      D       C   ;  D   E         G   7   +     L      ?*     O   +     R     �?+     Z       +     \     ��+     ^      +     n         �      "      �          +     �        �                  �      �   ;  �   �      +     �         �            �           �       
 �   �   �   �   �                 �   �     �   �      �      �   ;  �   �         �      �      �         +     �      +     �      +     �         �            �          ;  �   �                             ;         ;         +       �I@+       ��?+       �
�?+       �I?+       |� ?+       ���>+       ��"?+       Evt?+       ���?+       ��?+        ��@+     !  ��A+     "  �IA+     #  �S{A+     $  T�-@+     %  �Z�>+     &  ���.+     '  �O
+     (    �+     )     +     *     �  +                                ,  +    -  ,     .     -  ;  .  /     +     0  ����+     1  ��L>+     2  ���=+     3  fff?+     4  �bJ@+     5  ��>+     6    �@ 	 7                            8  7     9      8  ;  9  :         ;      G   ;  ;  <      ;  ;  =      ;  ;  >      6               �     ;     �      ;     �      ;  
   �      ;  
   �      ;     �      ;     �      ;     �      ;  �   �      ;           ;          ;          ;          ;          ;     
     ;          A  �   �   �   �   =     �   �   >  �   �   A  �   �   �   <   =     �   �   >  �   �   =     �   �   A  �   �   �   =   �   =  �   �   �   Q  �   �   �       A  �   �   �   =   Q     �   �       A  .   �   �   =   >  �   �   Q  �   �   �      A  �   �   �   �   Q     �   �       A  .   �   �   =   >  �   �   Q  �   �   �      A  �   �   �   <   Q     �   �       A  .   �   �   =   >  �   �   Q  �   �   �      A  �   �   �   �   Q     �   �       A  .   �   �   =   >  �   �   Q     �   �      A     �   �   �   >  �   �   Q     �   �      A  .   �   �   �   >  �   �   Q     �   �      A  .   �   �   �   >  �   �   Q     �   �      A  .   �   �   -   >  �   �   =  	   �   �   >  �   �   =     �   �   >  �   �   9     �      �   �   �  �       �  �   �   �   �  �   �  �  �   A  �   �   �   =   =     �   �        �      E   �   >  �   �   A  �   �   �   �   =     �   �        �      E   �   >  �   �   =     �   �   �     �   �   =     �   �   �     �   �   A      �   n   =         >       =       �   >      >    �   >    �   =     	  �   >    	  =       �   >  
    =       �   >      9  "     +              
    >  �     A  �     �   =   =   =          >      A  �     �   =   �   =          >      �  8  6               7  
      7        �     A  .   /      -   =     0   /   �     2   0   1   �     4   2   3   �  6       �  4   5   6   �  5   A  .   >      <   =   =     ?   >   A  @   A   ;   ?   =  7   B   A   =  C   F   E   V  G   H   B   F   =     I      W      J   H   I   Q     K   J       �     M   K   L   �  M   �  6   �  O   8  6               7        �     =     S      O     T   S   S               U         T   P     V   R   R   �     W   V   U   A     X      3   =     Y   X   �     [   Y   Z   �     ]   [   R   \   A     _      ^   =     `   _   �     a   `   Z   �     b   a   R   \   P     c   ]   b   �     d   W   c   �  d   8  6               7        �     ;     v      ;           A     g      3   =     h   g        i         h   A     j      ^   =     k   j        l         k   �     m   i   l   A     o      n   =     p   o        q         p   �     r   m   q   =     s      P     t   r   r   r   �     u   s   t   >     u   A     w      n   =     x   w   �     y   x   Z   �  {       �  y   z   ~   �  z   =     |      O     }   |   |          >  v   }   �  {   �  ~   =     �      O     �   �   �          >     �   9     �         >  v   �   �  {   �  {   =     �   v   =     �      O     �   �   �            >     �   =     �      O     �   �   �          �  �   8  6  "   +       #   7     $   7     %   7     &   7     '   7     (   7     )   7     *   �  ,   ;     �      ;     �      ;  .   �      ;     �      ;  .   �      ;     �      ;  �   �      =     �   &        �      :   �   |     �   �   >  �   �   =     �   '        �      :   �   |     �   �   >  �   �   =     �   (   >  �   �   9     �      �        �      8   �   >  �   �   =     �   )   >  �   �   9     �      �        �      8   �   >  �   �   =     �   $   =     �   *   |     �   �   |     �   �   =     �   �   =     �   �   P      �   �   �   �   �   A  �   �   �   =   =   >  �   �   =     �   %   =     �   �   |     �   �   =     �   �   |     �   �   Q     �   �       Q     �   �      P      �   �   �   �   �   A  �   �   �   =   �   >  �   �   =  "   �   �   �  �   8  