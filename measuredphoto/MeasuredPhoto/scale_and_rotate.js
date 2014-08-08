
////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////                        CODE TO DO PAN AND ZOOM ANIMATIONS , scalling,                 ////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////


var last_bounce_animation_time = 10;
var pan_bounce_frame_start = null;
var landscape_offset = 0;
var portrait_offset = 0;

function calculate_min_zoom(){
    var effective_width = img_container.width();
    var effective_height = img_container.height();
    if (last_orientation == 3 || last_orientation == 4){
        effective_width = img_container.height();
        effective_height = img_container.width();
    }
    
    var z_factor;
    if ((image_width)/(image_height) >= effective_width/effective_height) { //image is too wide for screen, scale by width
        console.log('scale by width');
        z_factor = effective_width / image_width;
    }
    else {
        console.log('scale by height')
        //image is too tall for screen, scale by hieght
        z_factor = effective_height / image_height;
    }
    return z_factor; //we never want to let the user go smaller than this.
}

function scaleImageToMatchScreen() {
    console.log('scale image to match screen');
    if (draw_g.node.contains(measured_svg.node)) {draw_g.node.removeChild(measured_svg.node);}
    var changing_orientation = false;
    
    if(window.innerHeight > window.innerWidth){
        portrait_offset = button_size;
        landscape_offset = 0;
        console.log('img_container sizing portrate');
        img_container.size(window.innerWidth, window.innerHeight - button_size);
        img_container.move(0,0);
        if (orientation_drawn_landsacep == true) {changing_orientation = true;}
        orientation_drawn_landsacep = false;
    }
    else {
        portrait_offset = 0;
        landscape_offset = button_size;
        console.log('img_container sizing landsape');
        img_container.size(window.innerWidth - button_size, window.innerHeight);
        img_container.move(button_size, 0);
        if (orientation_drawn_landsacep == false) {changing_orientation = true;}
        orientation_drawn_landsacep = true;
    }
    
    
    //scale image to match screen
    if (initial_load) {
        x_offset = img_container.width()/2;
        y_offset = img_container.height()/2;
        zoom_factor = calculate_min_zoom();
        min_zoom = zoom_factor; //we never want to let the user go smaller than this.
    }
    else { //TODO: comment why this is doing this... is it for forced ortientatino change? or tablet browser uncontroled orientation change?
        if (changing_orientation && orientation_drawn_landsacep) { //going from portrait to landscape
            x_offset = x_offset - (img_container.width() - prior_window_inner_width)/1.1;
        }
        else if (changing_orientation && !orientation_drawn_landsacep) { //going from landscape to portrait
            x_offset = x_offset - (img_container.width() - prior_window_inner_width)/2.5;
        }
    }
    prior_window_inner_width = img_container.width();
    prior_window_inner_height = img_container.height();
    
    calculate_zoom_boundaries(last_orientation); //set the max/min zoom offsets
    
    draw_g.scale(zoom_factor);
    drawing_pan_offset();
    
    
    //redraw_all_measurements(); //i think this is no longer necessary as we are doing image based location in the svg
    draw_g.node.appendChild(measured_svg.node);
    initial_load = false;
    
}


function shift_on_zoom(zoom_amount, center_x, center_y) {
    //the scale change will change the size of the image by a certain number of pixels given the current scale.
    //we need to change the offset such that the 'center' point stays in place.
    //this means multiplying the total amount of image size change in pixles by percentage of the images size it is away from the center point
    var i = pxl_to_img_xy(center_x, center_y);
    return {'x':(i.x - image_width/2)   * zoom_factor  * (zoom_amount - 1), 'y':(i.y - image_height/2)  * zoom_factor  * (zoom_amount - 1)};
}


function zoom(zoom_amount, center_x, center_y) {
    var start_time = new Date();
    
    if (zoom_factor < .9 * min_zoom && zoom_amount < 1){ zoom_amount = 1; } //limits amount off zoom out allowed
    
    var s = shift_on_zoom(zoom_amount, center_x, center_y);
    zoom_factor = zoom_factor * zoom_amount;
    x_offset = x_offset - s.x;
    y_offset = y_offset - s.y;
    
    var draw_start = new Date();
    
    scaleImageToMatchScreen();
    
    var draw_end = new Date();
    var m_time = (draw_start.getSeconds() - start_time.getSeconds()).toString() + "sec, " + (draw_start.getMilliseconds() - start_time.getMilliseconds()).toString() + " milisec"
    var d_time = (draw_end.getSeconds() - draw_start.getSeconds()).toString() + "sec, " + (draw_end.getMilliseconds() - draw_start.getMilliseconds()).toString() + " milisec"
}


function start_pan_bounce(){
    if (bounce_animation_id) {window.cancelAnimationFrame(bounce_animation_id)};
    pan_bounce_frame_start = new Date();
    bounce_animation_id = window.requestAnimationFrame(animate_pan_bounce);}


function animate_pan_bounce(unused_time) {
    if (pan_bounce_frame_start == null) { pan_bounce_frame_start = new Date();}
    
    if (( x_offset > max_x_offset) || ( x_offset < min_x_offset) ||
        ( y_offset > max_y_offset) || ( y_offset < min_y_offset))
    {
        if ((x_offset ) > max_x_offset ) { x_offset =  Math.max( x_offset - Math.max(last_bounce_animation_time, (x_offset - max_x_offset)/5), max_x_offset );}
        if ((x_offset ) < min_x_offset ) { x_offset =  Math.min( x_offset + Math.max(last_bounce_animation_time, (min_x_offset - x_offset)/5), min_x_offset );}
        if ((y_offset ) > max_y_offset ) { y_offset =  Math.max( y_offset - Math.max(last_bounce_animation_time, (y_offset - max_y_offset)/5), max_y_offset );}
        if ((y_offset ) < min_y_offset ) { y_offset =  Math.min( y_offset + Math.max(last_bounce_animation_time, (min_y_offset - y_offset)/5), min_y_offset );}
        drawing_pan_offset();
        bounce_animation_id = window.requestAnimationFrame(animate_pan_bounce);
    }
    else { if (bounce_animation_id) { window.cancelAnimationFrame(bounce_animation_id); } }
    
    var draw_end = new Date();
    if (draw_end.getMilliseconds() - pan_bounce_frame_start.getMilliseconds() > 0 ) {last_bounce_animation_time = draw_end.getMilliseconds() - pan_bounce_frame_start.getMilliseconds();}
    pan_bounce_frame_start = new Date();
    
}

var zoom_animation_id = null;
var last_zoom_animation_time = 10;
var zoom_frame_start = null;

function start_zoom_return(){
    if (zoom_animation_id) { return null; }
    else {
        zoom_frame_start = new Date();
        zoom_animation_id = window.requestAnimationFrame(animate_zoom_return);
    }
}


function animate_zoom_return(unused_time) {
    if (zoom_frame_start == null) { zoom_frame_start = new Date();}
    if (zoom_factor < min_zoom)
    {
        zoom( 1 + last_bounce_animation_time/1000, img_container.width() / 2, img_container.height() / 2);
        zoom_animation_id = window.requestAnimationFrame(animate_zoom_return);
    }
    else { if (zoom_animation_id) {
        window.cancelAnimationFrame(zoom_animation_id);
        zoom_animation_id = null;
        start_pan_bounce();
    } }
    
    var draw_end = new Date();
    if (draw_end.getMilliseconds() - zoom_frame_start.getMilliseconds() > 0 ) {last_bounce_animation_time = draw_end.getMilliseconds() - zoom_frame_start.getMilliseconds();}
    zoom_frame_start = new Date();
    
}


function calculate_zoom_boundaries(orientation) {
    //calculate zoom offsets
    //if (orientation == 1 || orientation == 2) { //coordinate system is not rotated... origin is in upper left of screen
    max_x_offset = Math.max( image_width*zoom_factor/2, img_container.width()/2   );
    max_y_offset = Math.max( image_height*zoom_factor/2,  img_container.height()/2 );
    
    if (img_container.width() <  image_width * zoom_factor) {
        min_x_offset = img_container.width() - image_width * zoom_factor/2;}
    else { min_x_offset = Math.max( image_width * zoom_factor/2, img_container.width()/2   );}
    
    if (img_container.height() < image_height * zoom_factor) {
        min_y_offset = img_container.height() - image_height * zoom_factor / 2;}
    else { min_y_offset = Math.max( portrait_offset - image_height * zoom_factor / 2,  img_container.height()/2);}
    //}
    if (orientation == 3) { //origin is rotaed + 90% from center of screen.... potentially plus or minus some prior offset
        if (image_width * zoom_factor > img_container.height()){
            max_x_offset = img_container.width()/2 - img_container.height()/2 + image_width*zoom_factor/2;
            min_x_offset = img_container.width()/2 + img_container.height()/2 - image_width*zoom_factor/2;
        }
        if (image_height * zoom_factor > img_container.width()){
            max_y_offset = img_container.height()/2 - img_container.width()/2 + image_height*zoom_factor/2;
            min_y_offset = img_container.height()/2 + img_container.width()/2 - image_height*zoom_factor/2;
        }
    }
    else if (orientation == 4) { //origin is rotaed - 90% from center of screen.... potentially plus or minus some offset
        if (image_width * zoom_factor > img_container.height()){
            max_x_offset = img_container.width()/2 - img_container.height()/2 + image_width*zoom_factor/2;
            min_x_offset = img_container.width()/2 + img_container.height()/2 - image_width*zoom_factor/2;
        }
        if (image_height * zoom_factor > img_container.width()){
            max_y_offset = img_container.height()/2 - img_container.width()/2 + image_height*zoom_factor/2;
            min_y_offset = img_container.height()/2 + img_container.width()/2 - image_height*zoom_factor/2;
        }
    }
    
}

function drawing_pan_offset() {
    // slow down the drag if ouside boundry
    var x, y;
    if ((x_offset ) > max_x_offset ) {
        x = max_x_offset + ( x_offset - max_x_offset)/2; }
    else if ((x_offset ) < min_x_offset) {
        x = min_x_offset + ( x_offset - min_x_offset)/2;}
    else {
        x = x_offset;
    }
    if ((y_offset ) > max_y_offset) {
        y = max_y_offset + ( y_offset - max_y_offset) /2 ;}
    else if ((y_offset ) < min_y_offset) {
        y = min_y_offset + ( y_offset - min_y_offset)/2;}
    else {
        y = y_offset;
    }
    draw_g.center(x,y);
    
}

// helper method to keep track of how screen is changing with regrads zoom and pan
function pxl_to_img_xy(pX, pY){
    var iX, iY;
    console.log(last_orientation);
    if (last_orientation == 1) { //regular portrait
        iX = (pX - x_offset - img_container.x())/zoom_factor + image_width/2; //we add half image width because x_offset is relative to image center
        iY = (pY - y_offset)/zoom_factor + image_height/2;
    }
    else if (last_orientation == 2) { //upside down portrait
        iX = (img_container.width() - pX - x_offset+ img_container.x())/zoom_factor + image_width/2;
        iY = (img_container.height()  - pY - y_offset)/zoom_factor + image_height/2;
    }
    else if (last_orientation == 3 ) {
        iX = (pY - x_offset + (img_container.width() - img_container.height())/2)/zoom_factor + image_width/2;
        iY = ((img_container.width() - pX) + img_container.x() - y_offset + (img_container.height() - img_container.width())/2)/zoom_factor + image_height/2; //the top of the image is to the right of the screen
    }
    else {  //last orientation == 4
        iX = ((img_container.height() - pY) - x_offset + (img_container.width() - img_container.height())/2)/zoom_factor + image_width/2;
        iY = (pX - img_container.x() - y_offset + (img_container.height() - img_container.width())/2)/zoom_factor + image_height/2;
    }
    return {'x':iX, 'y':iY};
}

