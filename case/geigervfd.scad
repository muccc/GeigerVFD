use<knob.scad>;

eps=0.01;

pcb_x = 109.7;
pcb_y = 117.35;
pcb_z = 1.6;

pcb_pad_top = 3;
pcb_pad_bottom = 27+5;
pcb_fit=1;

pcb_mount_holes = [ [23.2,29.46],
                    [23.2,76.32], 
                    [90.42,76.32] ];

pcb_led_pos = [23.2,62.74];//78.99];

pcb_mount_through = true;

pcb_mount_hole_d = 4;

case_wall_side = 2;

case_body_z = pcb_z+pcb_pad_top+pcb_pad_bottom;
//case_cover_h=5;

case_xl_oversize=3.5;
case_xr_oversize=5;

case_r=4.5;
case_y_oversize = 25;
case_cutout_dim=[pcb_x+2*pcb_fit+case_xl_oversize+case_xr_oversize,pcb_y+2*pcb_fit+case_y_oversize,case_body_z+2*eps];
case_body_dim=[case_cutout_dim[0]+2*case_wall_side,case_cutout_dim[1
]+2*case_wall_side,case_body_z];

case_offset = case_wall_side+pcb_fit;

case_top_dim=[case_body_dim[0],case_body_dim[1],case_r];
case_mount_d = 12;
case_mount_bore=1.5;
cover_mount_bore=2;

case_mounts=[[0,0,+1.5,+1.5],
            [0,case_body_dim[1],1.5,-1.5],
            [case_body_dim[0],0,-1.5,1.5],
            [case_body_dim[0],case_body_dim[1],-1.5,-1.5],
            [case_body_dim[0],case_body_dim[1]/2,-1,0],
            [0,case_body_dim[1]/2,1,0]
            ];

explode = 0;

usb_dim=[12,6];
usb_pos = [1,28.42/2+2.03+5];

has_buzzer=true;

display_size=[107,22,23];
display_pos=[1.3,92.38,-eps];

encoder_pos=[56.39,39.62,-eps];
encoder_bore=8;

module pcb(){
    translate([case_xl_oversize,0,0]) difference(){
        cube([pcb_x,pcb_y,pcb_z]);
        translate([0,0,-eps]) for (hole_pos=pcb_mount_holes)
            translate(hole_pos) cylinder(d=pcb_mount_hole_d, h=pcb_z+2*eps);
    }
}

module ring(d_ring, h_ring){
    difference(){
        cylinder(d1=d_ring+h_ring*3,d2=d_ring+h_ring*2,h=h_ring,$fn=64);
        cylinder(d1=d_ring,d2=d_ring+h_ring,h=h_ring+eps,$fn=64);
    }
}

module cube_rounded(dim,r,center=false){
    co = center?[0,0,-dim[2]/2]:[dim[0]/2,dim[1]/2];
    
    translate(co) hull() for(mx=[0,1], my=[0,1]) 
        mirror([mx,0,0])mirror([0,my,0])
        translate([(dim[0]/2)-r,(dim[1]/2)-r])
        cylinder(r=r, h=dim[2]);
}

module cube_cover(dim,center=false,invert=false) {
    co = center?[0,0,-dim[2]/2]:[dim[0]/2,dim[1]/2];
    r=dim[2];
    zz = invert?-dim[2]:0;
    translate(co) hull() for(mx=[0,1], my=[0,1]) 
        mirror([mx,0,0])mirror([0,my,0])
        translate([(dim[0]/2)-r,(dim[1]/2)-r])
        intersection(){
            sphere(r=r);
            translate([0,0,zz]) cylinder(r=dim[2],h=dim[2]);
        }
}

module body(){
    translate([0,0,-pcb_pad_bottom]) difference(){
        translate([-case_offset,-case_offset]){
        //union(){
            
            cube_rounded(case_body_dim,r=case_r);
            
            hull() for(ix=[0,1],iy=[0,1])translate([ix*case_body_dim[0],iy*case_body_dim[1],case_body_z/2-5]) sphere(r=case_r-0.5);
            
            for(mount = case_mounts)
                translate([mount[0],mount[1]]) mount_body(case_mount_d,case_body_z,mount[2],mount[3]);
        }
        translate([-pcb_fit,-pcb_fit,-eps]) cube(case_cutout_dim);
        
        
        //usb
        translate([pcb_x,usb_pos[1],pcb_pad_bottom-usb_dim[1]/2-usb_pos[0]])rotate([00,90,0]) hull() for(i=[-1,1]) translate([0,i*usb_dim[0]/2,0]) cylinder(d=usb_dim[1],h=60,center=true,$fn=16);
    }
}

module mount_body(d,h,dx=0,dy=0){
    difference(){
        hull(){
            cylinder(d=d,h=h);
            translate([dx*d,dy*d,0]) cylinder(d=d,h=h);
        }
        translate([0,0,-eps]) cylinder(d=case_mount_bore,h=h+2*eps,$fn=16);
    }
}

module mount_top(d,dx=0,dy=0,invert=false) {
    m=invert?1:0;
    mirror([0,0,m]) difference(){
        hull(){
            intersection(){
                sphere(d=d);
                cylinder(d=d,h=d);
            }
            translate([dx*d,dy*d,0]) intersection(){
                 sphere(d=d);
                 cylinder(d=d,h=d);
            }
        }
        translate([0,0,-eps]) cylinder(d=cover_mount_bore,h=d+2*eps,$fn=16);
        translate([0,0,d/2-case_mount_bore]) cylinder(d1=case_mount_bore,d2=cover_mount_bore*2,h=case_mount_bore,$fn=16);
    }
}
module cover(is_bottom=false){
    oz= is_bottom?-pcb_pad_bottom:pcb_pad_top+pcb_z;
    translate([-case_offset,-case_offset,oz]){
        difference(){
            cube_cover(case_top_dim,invert=is_bottom);
            
            for(i=[4,5]) translate([case_mounts[i][0],case_mounts[i][1],-10])cylinder(d=cover_mount_bore,h=20);
            
        }
            for(mount = case_mounts)
                translate([mount[0],mount[1]]) mount_top(case_mount_d,mount[2],mount[3],invert=is_bottom);
    }
}

module cover_bottom(){
    cover(is_bottom=true);
}

module cover_top() {
    z_top = case_r+pcb_pad_top;
    difference(){
        union(){
            cover(is_bottom=false);
            
            over =7.2;
            
            //display
            translate([case_xl_oversize,0,0])translate([-over/2,-over/2,z_top-0.5]) translate(display_pos) cube_cover([display_size[0]+over,display_size[1]+over,4]);
            
            //buzzer
            if(has_buzzer) translate([case_xl_oversize,0,0]) translate([60,112,z_top]) cube_cover([30,20,3.5]);
            
            //encoder
            translate([case_xl_oversize,0,0]) translate([0,0,z_top]) translate(encoder_pos) ring(42,5);
            
            //led
            translate([case_xl_oversize,0,0]) translate([0,0,z_top]) translate(pcb_led_pos) ring(6,5);
        }
        translate([case_xl_oversize,0,0]) translate(display_pos) cube(display_size);
            
        over2 = 1;
        //display
        translate([case_xl_oversize,0,0]) translate(display_pos) hull(){
            translate([0,0,z_top]) cube([display_size[0],display_size[1],eps]);
            translate([-over2/2,-over2/2,z_top+3])cube([display_size[0]+over2,display_size[1]+over2,eps]);
        }
        translate([case_xl_oversize,0,0]) translate(encoder_pos) cylinder(d=encoder_bore,h=23);
        translate([case_xl_oversize,0,0]) translate([0,0,-eps]) for (mount=pcb_mount_holes) translate(mount) cylinder(d=pcb_mount_hole_d,h=case_r+2*eps+10,$fn=16);
            
        translate([pcb_x/2,80,z_top]) linear_extrude(2)color("black") text("GEIGER VFD",halign="center", font="StalinistOne",size=9);
        
        translate([case_xl_oversize,0,0]) translate(pcb_led_pos) cylinder(d=6,h=case_r+2*eps+10,$fn=16);
    
        if(has_buzzer) translate([case_xl_oversize,0,0]) translate([60,111.5,z_top])
            for(i=[0:2]) {
                translate([0,i*4+7,0]) 
                hull() for(j=[0,1]) translate([j*20+5,0,0]) cylinder(d=2,h=10,$fn=16,center=true);
        }
    }
    if(false) translate([0,0,pcb_z]) for (mount=pcb_mount_holes) translate(mount) difference(){
        cylinder(d=pcb_mount_hole_d*2,h=pcb_pad_top,$fn=16);
        #translate([0,0,-eps]) cylinder(d=pcb_mount_hole_d,h=pcb_pad_top,$fn=16);
    }
}

//color("green") pcb();
color("red") body();

translate([0,0,explode])  cover_top();
translate([0,0,-explode]) color("orange") cover_bottom();
translate([case_xl_oversize,0,0]) translate([encoder_pos[0],encoder_pos[1], pcb_pad_top+case_r+1*explode]) knob();