/* Open SCAD Name.: BagDispenser_v1.scad
*  Copyright (c)..: 2017 www.DIY3DTech.com
*
*  Creation Date..: July-2017
*  Description....: Code to create pegboard bag dispenser
*
*  Rev 1: Develop Model
*  Rev 2: 
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*/ 

/*------------------Customizer View-------------------*/

// preview[view:north, tilt:top]

/*---------------------Parameters---------------------*/

//board width in mm
board_width         =   30; //[1:1:500]

//board height in mm
board_height        =   30 ; //[1:1:500]

//board thickness in mm
board_thick         =     1.5; //[1:0.5:10]
/*-----------------------Execute----------------------*/

main_module();

/*-----------------------Modules----------------------*/

module main_module(){ //create module
    difference() {
            union() {//start union
           cube([board_width,board_height,board_thick],false);     
                
            } //end union

    
     } //end difference
         
} //end module
           
/*----------------------End Code----------------------*/