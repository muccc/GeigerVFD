eps = 0.01;
$fn=64;

knob_d = 38;
knob_h = 15;
knob_chamfer=2;

grab_n = 8;
grab_d = 10;
grab_offset=3;

back_d=knob_d-10;
back_h=knob_h-4;

axis_h=0.1;
axis_d=6;
axis_d_flat=5;
axis_flat_offset=4;
axis_d_i=3.2;



module engraving(){
    translate([-0.4,0.5]) linear_extrude(2) scale([0.055,0.055]) import("rad.svg");
    //for(dia=[13,26]) difference(){
    //    cylinder(d=dia,h=2);
    //    translate([0,0,eps]) cylinder(d=dia-1,h=2);
    //}
}

module cylinder_chamfered(d,h,c,c_out=false){
    cdir= c_out?-1:1;
    cylinder(d=d,h=h+eps-c);
    translate([0,0,h-c]) cylinder(d1=d,d2=d-(cdir*c*2),h=c);
}

module knob(){
    difference(){
        union(){
            //main
            cylinder_chamfered(h=knob_h, d=knob_d, c=knob_chamfer);
        }
        
        //grab cutouts
        for(az=[0:360/grab_n:360]) rotate([0,0,az]) translate([knob_d/2+grab_offset,0,-eps]) cylinder_chamfered(d=grab_d,h=knob_h+2*eps,c=knob_chamfer, c_out=true);
    
        //back cutout
        translate([0,0,-eps]) cylinder(d=back_d,h=back_h);
        
        translate([0,0,knob_h-0.5]) engraving();
    }
    
    //axis
    
    
    difference(){
        union(){
            for(az=[0,90]) rotate([0,0,az]) translate([0,0,back_h/2]) cube([back_d+eps,1,back_h],center=true);

            translate([0,0,-axis_h+eps]) cylinder(d=axis_d+4,h=axis_h-eps);
            cylinder(d2=axis_d*2+4,d1=axis_d+4,h=back_h);

        }
        
        difference(){
            translate([0,0,-axis_h]) cylinder(d=axis_d,h=axis_h+knob_h);
                
            translate([axis_d_flat,0,-(axis_h)/2+(knob_h/2)]) cube([axis_d,axis_d,axis_h+knob_h+eps],center=true);
            
        }
    }
}

rotate([180,0,0]) knob();