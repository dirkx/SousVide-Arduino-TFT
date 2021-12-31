
screen_mod_x=72.5; 
screen_mod_y=52.7;

screen_x=50;
screen_y=37;

uno_x=69;
uno_y=53.5;

height=40;

wall=1;
insert=5;

W=max(screen_x, uno_x) + wall * 2;
H=max(screen_y, uno_y) + wall * 2;

$fn=50;

module screwinsert() {
   difference() {
    minkowski() {
         cube([insert+2*wall, insert+2*wall,2.5*insert]);
         sphere(r=wall);
    };
    translate([insert/2+wall,insert/2+wall,insert-wall]) cylinder(r=insert/2,2.5*insert,center=true);
};
 };
 
module box(h=height,offset=-wall*2) {
    union() {
    difference()  {
        cube([W,H,h], center=true);
        translate([0,0,offset]) cube([W-2*wall,H-2*wall,2*h], center=true);
    };
};   
}

module topbox() {
    
difference(h=wall) {
    box(h);
    translate([0,0,h/2-wall]) cube([screen_x,screen_y,3*wall], center=true);
};
};

module bottombox() {
   union() {
   box(height,-10);
   translate([-W/2+wall,-H/2+wall,-height/2+wall]) screwinsert();
   translate([-W/2+wall,+H/2-insert-3*wall,-height/2+wall]) screwinsert();
   translate([+W/2-insert-3*wall,-H/2+wall,-height/2+wall]) screwinsert();
   translate([+W/2-insert-3*wall,+H/2-insert-3*wall,-height/2+wall]) screwinsert();
    };
 }
 
module all() {
difference() {
    union() { 
        bottombox(); 
        translate([0,0,10]) rotate([15,0,0]) topbox(); 
    };
    translate([0,0,31.22325]) rotate([15,0,0]) cube([2*W,2*H,height], center=true);

 };
 };
 
 { all(); sphere(r=1); };
