
namespace eval ::moonlib::image:: {
}

proc ::moonlib::image::configure {obj_id img filename} {
    if { [catch {$img configure -file $filename} fid]} {
        ::pdwindow::logpost $obj_id 1 "[image]: error reading '$filename':\n$fid\n"
    }
}

proc ::moonlib::image::create_photo {obj_id img filename} {
    if { [catch {image create photo $img -file $filename} fid]} {
        ::pdwindow::logpost $obj_id 1 "[image]: error reading '$filename':\n$fid\n"
    }
}
