 ފ          #     j              K  
 	 SPV_KHR_shader_draw_parameters       GLSL.std.450              	        main    $   J   L   T        �   	 GL_ARB_shader_draw_parameters    
 GL_GOOGLE_cpp_style_line_directive       main      
   mesh_descriptor  	 
       model_transform_matrix   	 
      tangent_transform_quat    
      bounding_sphere   
      matIdx    
      light_caster      
      _unused0      
      _unused1         transform_model(struct-mesh_descriptor-mf34-vf4-vf4-i1-i1-f1-f11;vf3;        mesh         v     "   draw_id   $   gl_BaseInstanceARB    '   md    (   mesh_descriptor  	 (       model_transform_matrix   	 (      tangent_transform_quat    (      bounding_sphere   (      matIdx    (      light_caster      (      _unused0      (      _unused1     	 *   mesh_descriptors_binding     	 *       mesh_descriptor_buffer    ,         H   vs_out    H       position      H      instanceIdx   H      drawIdx   J   vout      L   vert      M   param     O   param     T   gl_InstanceIndex      [   mesh_draw_params      [       count     [      first_index   [      base_vertex   [      _unused  	 ]   mesh_draw_params_binding     	 ]       mesh_draw_params_buffer   _         `   dual_quaternion   `       real      `      dual     
 a   view_transform_buffer_struct      a       view_transform   	 a      inverse_view_transform    a      eye_position     
 b   view_transform_buffer_binding    	 b       view_transform_buffer     d        
 f   proj_transform_buffer_struct      f       proj_xywz     f      backbuffer_size   f      tan_half_fovy     f      aspect   
 g   proj_transform_buffer_binding    	 g       proj_transform_buffer     i       G  $      I  H  (          H  (          H  (          H  (       #       H  (             H  (         H  (         H  (      #   0   H  (         H  (         H  (      #   @   H  (         H  (         H  (      #   P   H  (         H  (         H  (      #   T   H  (         H  (         H  (      #   X   H  (         H  (         H  (      #   \   G  )      `   H  *          H  *          H  *       #       G  *      G  ,   "      G  ,   !      H  H         H  H         G  H      G  L         G  T      +   H  [          H  [          H  [       #       H  [         H  [         H  [      #      H  [         H  [         H  [      #      H  [         H  [         H  [      #      G  \         H  ]          H  ]          H  ]       #       G  ]      G  _   "      G  _   !      H  `       #       H  `      #      H  a          H  a          H  a       #       H  a         H  a         H  a      #       H  a         H  a         H  a      #   @   H  b          H  b          H  b       #       G  b      G  d   "      G  d   !       H  f          H  f          H  f       #       H  f         H  f         H  f      #      H  f         H  f         H  f      #      H  f         H  f         H  f      #      H  g          H  g          H  g       #       G  g      G  i   "      G  i   !           !                                         	           	 
            	   	                  
                          !              +          �?+  	                                       !             #      	   ;  #   $       	 (            	   	           )   (     *   )      +      *   ;  +   ,         .      (   +  	   4         5         +  	   8      +  	   ;         <      	   +  	   ?      +  	   B         C         +  	   F        H      	          I      H   ;  I   J         K         ;  K   L         R         ;  #   T         V      	      Y            [                    \   [     ]   \      ^      ]   ;  ^   _        `           a   `   `        b   a      c      b   ;  c   d        e            f      e           g   f      h      g   ;  h   i      6               �     ;  !   "      ;     '      ;     M      ;     O      =  	   %   $   |      &   %   >  "   &   =      -   "   A  .   /   ,      -   =  (   0   /   Q     1   0       A     2   '      >  2   1   Q     3   0      A  5   6   '   4   >  6   3   Q     7   0      A  5   9   '   8   >  9   7   Q  	   :   0      A  <   =   '   ;   >  =   :   Q  	   >   0      A  <   @   '   ?   >  @   >   Q     A   0      A  C   D   '   B   >  D   A   Q     E   0      A  C   G   '   F   >  G   E   =  
   N   '   >  M   N   =     P   L   >  O   P   9     Q      M   O   A  R   S   J      >  S   Q   =  	   U   T   A  V   W   J   4   >  W   U   =      X   "   A  Y   Z   J   8   >  Z   X   �  8  6               7        7        �     =           Q               Q              Q              P                    A              =           �              �     8  