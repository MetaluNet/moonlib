# For information on usage and redistribution, and for a DISCLAIMER OF ALL
# WARRANTIES, see the file, "LICENSE.txt," in this distribution.
# Copyright (c) 1997-2009 Miller Puckette.

# this is an adapted copy of pd/tcl/dialog_iemgui.tcl

package provide dialog_mknob 0.1

namespace eval ::dialog_mknob:: {
    namespace export mknob_properties
}
# some constants
set ::dialog_mknob::min_flashhold 50
set ::dialog_mknob::min_flashbreak 10
set ::dialog_mknob::min_fontsize 4

# arrays to store per-dialog values
array set ::dialog_mknob::var_width {} ;
array set ::dialog_mknob::var_height {} ;
array set ::dialog_mknob::var_minwidth {} ;
array set ::dialog_mknob::var_minheight {} ;

array set ::dialog_mknob::var_range_max {} ;
array set ::dialog_mknob::var_range_min {} ;
array set ::dialog_mknob::var_range_checkmode {} ;

array set ::dialog_mknob::var_mode {} ;
array set ::dialog_mknob::var_loadbang {} ;
array set ::dialog_mknob::var_steady {} ;
array set ::dialog_mknob::var_number {} ;

array set ::dialog_mknob::var_snd {} ;
array set ::dialog_mknob::var_rcv {} ;
array set ::dialog_mknob::var_label {} ;

array set ::dialog_mknob::var_label_dx {} ;
array set ::dialog_mknob::var_label_dy {} ;
array set ::dialog_mknob::var_label_font {} ;
array set ::dialog_mknob::var_label_fontsize {} ;

array set ::dialog_mknob::var_color_background {} ;
array set ::dialog_mknob::var_color_foreground {} ;
array set ::dialog_mknob::var_color_label {} ;
array set ::dialog_mknob::var_colortype {} ;

#### original values (to fall back from invalid ones))
array set ::dialog_mknob::old_width {}
array set ::dialog_mknob::old_height {}
array set ::dialog_mknob::old_range_max {}
array set ::dialog_mknob::old_range_min {}
array set ::dialog_mknob::old_number {}
array set ::dialog_mknob::old_label_dx {}
array set ::dialog_mknob::old_label_dy {}
array set ::dialog_mknob::old_label_font {}
array set ::dialog_mknob::old_label_fontsize {}

# TODO convert Init/No Init and Steady on click/Jump on click to checkbuttons

proc ::dialog_mknob::tonumber val {
    set x 0
    catch {
        set x [expr $val]
    }
    return $x
}

proc ::dialog_mknob::clip {val min {max {}}} {
    set val [tonumber $val]
    if {$min ne {} && $val < $min} {return $min}
    if {$max ne {} && $val > $max} {return $max}
    return $val
}

proc ::dialog_mknob::clip_dim {mytoplevel} {
    set vid [string trimleft $mytoplevel .]

    set ::dialog_mknob::var_width($vid) [clip $::dialog_mknob::var_width($vid) $::dialog_mknob::var_minwidth($vid)]
    set ::dialog_mknob::var_height($vid) [clip $::dialog_mknob::var_height($vid) $::dialog_mknob::var_minheight($vid)]
}

proc ::dialog_mknob::clip_num {mytoplevel} {
    set vid [string trimleft $mytoplevel .]

    set ::dialog_mknob::var_number($vid) [clip $::dialog_mknob::var_number($vid) 1 2000]
}

proc ::dialog_mknob::sched_rng {mytoplevel} {
    # TODO: rename this to 'range_check'
    set vid [string trimleft $mytoplevel .]
    switch -- $::dialog_mknob::var_range_checkmode($vid) {
        2 {
            # 'range-check' is in 'flash' mode
            # make sure that min/max are sorted properly and are not smaller than the resp. min values
            foreach {flashbreak flashhold} [lsort -real [list [tonumber $::dialog_mknob::var_range_min($vid)] [tonumber $::dialog_mknob::var_range_max($vid)]]] {break;}
            set ::dialog_mknob::var_range_min($vid) [clip $flashbreak $::dialog_mknob::min_flashbreak]
            set ::dialog_mknob::var_range_max($vid) [clip $flashhold $::dialog_mknob::min_flashhold]
        }
        1 {
            # 'range-check' is in 'toggle' mode
            if {[tonumber $::dialog_mknob::var_range_min($vid)] == 0} {
                #  there's little use toggling between 0 and 0, so force it to 1...
                set ::dialog_mknob::var_range_min($vid) 1
            }
        }
    }
}

proc ::dialog_mknob::verify_rng {mytoplevel} {
    set vid [string trimleft $mytoplevel .]

    if {$::dialog_mknob::var_mode($vid) == 1} {
        if {$::dialog_mknob::var_range_max($vid) == 0.0 && $::dialog_mknob::var_range_min($vid) == 0.0} {
            set ::dialog_mknob::var_range_max($vid) 1.0
        }
        if {$::dialog_mknob::var_range_max($vid) > 0} {
            if {$::dialog_mknob::var_range_min($vid) <= 0} {
                set ::dialog_mknob::var_range_min($vid) [expr $::dialog_mknob::var_range_max($vid) * 0.01]
            }
        } else {
            if {$::dialog_mknob::var_range_min($vid) > 0} {
                set ::dialog_mknob::var_range_max($vid) [expr $::dialog_mknob::var_range_min($vid) * 0.01]
            }
        }
    }
}

proc ::dialog_mknob::clip_fontsize {mytoplevel} {
    set vid [string trimleft $mytoplevel .]

    set ::dialog_mknob::var_label_fontsize($vid) [clip $::dialog_mknob::var_label_fontsize($vid) $::dialog_mknob::min_fontsize]
}

proc ::dialog_mknob::set_col_example {mytoplevel} {
    set vid [string trimleft $mytoplevel .]

    set fgcol $::dialog_mknob::var_color_label($vid)
    $mytoplevel.colors.sections.exp.lb_bk configure \
        -background $::dialog_mknob::var_color_background($vid) \
        -activebackground $::dialog_mknob::var_color_background($vid) \
        -foreground $fgcol -activeforeground $fgcol

    set fgcol $::dialog_mknob::var_color_foreground($vid)
    if { $fgcol eq "none" } {
        set fgcol $::dialog_mknob::var_color_background($vid)
    }
    $mytoplevel.colors.sections.exp.fr_bk configure \
        -background $::dialog_mknob::var_color_background($vid) \
        -activebackground $::dialog_mknob::var_color_background($vid) \
        -foreground $fgcol -activeforeground $fgcol

    # for OSX live updates
    if {$::windowingsystem eq "aqua"} {
        ::dialog_mknob::apply_and_rebind_return $mytoplevel
    }
}

proc ::dialog_mknob::preset_col {mytoplevel presetcol} {
    set vid [string trimleft $mytoplevel .]

    switch -- $::dialog_mknob::var_colortype($vid) {
        0 { set ::dialog_mknob::var_color_background($vid) $presetcol }
        1 { set ::dialog_mknob::var_color_foreground($vid) $presetcol }
        2 { set ::dialog_mknob::var_color_label($vid) $presetcol }
    }

    ::dialog_mknob::set_col_example $mytoplevel
}

proc ::dialog_mknob::choose_col_bkfrlb {mytoplevel} {
    # TODO rename this
    set vid [string trimleft $mytoplevel .]

    switch -- $::dialog_mknob::var_colortype($vid) {
        0 {
            set title [_ "Background color" ]
            set color $::dialog_mknob::var_color_background($vid)
        }
        1 {
            set title [_ "Foreground color" ]
            set color $::dialog_mknob::var_color_foreground($vid)
        }
        2 {
            set title [_ "Label color" ]
            set color $::dialog_mknob::var_color_label($vid)
        }
    }
    set color [tk_chooseColor -title $title -initialcolor $color]
    if { $color ne "" } {
        ::dialog_mknob::preset_col $mytoplevel $color
    }
}

proc ::dialog_mknob::popupmenu {path varname labels {command {}}} {
    upvar 1 $varname var

    menubutton ${path} -menu ${path}.menu -indicatoron 1 -relief raised -text [lindex $labels $var]
    menu ${path}.menu -tearoff 0
    set idx 0
    foreach l $labels {
        $path.menu add radiobutton -label "$l" -variable $varname -value $idx
        $path.menu entryconfigure last -command "\{$path\} configure -text \{$l\}; $command"
        incr idx
    }
}

proc ::dialog_mknob::toggle_mode {mytoplevel} {
    set vid [string trimleft $mytoplevel .]
    if {$::dialog_mknob::var_mode($vid) != 0} {
        ::dialog_mknob::verify_rng $mytoplevel
        ::dialog_mknob::sched_rng $mytoplevel
    }
}
proc ::dialog_mknob::toggle_and_activate {mytoplevel activewidget} {
    ::dialog_mknob::toggle_mode $mytoplevel
    set vid [string trimleft $mytoplevel .]

    if {$::dialog_mknob::var_mode($vid) != 0} {
        $activewidget configure -state normal
    } else {
        $activewidget configure -state disabled
    }
}

# open popup over source button
proc ::dialog_mknob::font_popup {mytoplevel} {
    $mytoplevel.popup unpost
    set button $mytoplevel.label.fontpopup_label
    set x [expr [winfo rootx $button] + ( [winfo width $button] / 2 )]
    set y [expr [winfo rooty $button] + ( [winfo height $button] / 2 )]
    tk_popup $mytoplevel.popup $x $y 0
}

proc ::dialog_mknob::toggle_font {mytoplevel gn_f} {
    set vid [string trimleft $mytoplevel .]
    set ::dialog_mknob::var_label_font($vid) $gn_f

    switch -- $gn_f {
        0 { set current_font $::font_family}
        1 { set current_font "Helvetica" }
        2 { set current_font "Times" }
    }
    set current_font_spec "{$current_font} 14 $::font_weight"

    $mytoplevel.label.fontpopup_label configure -text $current_font \
        -font [list $current_font 16 $::font_weight]
    $mytoplevel.label.name_entry configure -font $current_font_spec
    $mytoplevel.colors.sections.exp.fr_bk configure -font $current_font_spec
    $mytoplevel.colors.sections.exp.lb_bk configure -font $current_font_spec
}

proc ::dialog_mknob::apply {mytoplevel} {
    set vid [string trimleft $mytoplevel .]

    set fallbacks {}
    set prefix ::dialog_mknob::old_
    foreach v [info vars ${prefix}*] {
        if { ! [array exists $v] } {continue}
        lappend fallbacks [string range $v [string length $prefix] end]
    }

    foreach v ${fallbacks} {
        if { [lindex [array get ::dialog_mknob::var_${v} ${vid}] 1] eq {} } {
            array set ::dialog_mknob::var_${v} [array get ::dialog_mknob::old_${v} ${vid}]
        }
    }

    ::dialog_mknob::clip_dim $mytoplevel
    ::dialog_mknob::clip_num $mytoplevel
    ::dialog_mknob::sched_rng $mytoplevel
    ::dialog_mknob::verify_rng $mytoplevel
    ::dialog_mknob::sched_rng $mytoplevel
    ::dialog_mknob::clip_fontsize $mytoplevel


    # TODO wrap the name-mangling ('empty', unspace_text, map) into a helper-proc
    set sendname empty
    set receivename empty
    set labelname empty

    if {$::dialog_mknob::var_snd($vid) ne ""} {set sendname $::dialog_mknob::var_snd($vid)}
    if {$::dialog_mknob::var_rcv($vid) ne ""} {set receivename $::dialog_mknob::var_rcv($vid)}
    if {$::dialog_mknob::var_label($vid) ne ""} {set labelname $::dialog_mknob::var_label($vid)}

    set labelname [string map { "\\" "" {$} {\$} { } {\ } {,} {\,} {;} {\;}  "{" "\{" "}" "\}" } $labelname]

    # make sure the offset boxes have a value
    if {$::dialog_mknob::var_label_dx($vid) eq ""} {set ::dialog_mknob::var_label_dx($vid) 0}
    if {$::dialog_mknob::var_label_dy($vid) eq ""} {set ::dialog_mknob::var_label_dy($vid) 0}

    pdsend [list $mytoplevel dialog \
                $::dialog_mknob::var_width($vid) \
                $::dialog_mknob::var_height($vid) \
                $::dialog_mknob::var_range_min($vid) \
                $::dialog_mknob::var_range_max($vid) \
                $::dialog_mknob::var_mode($vid) \
                $::dialog_mknob::var_loadbang($vid) \
                $::dialog_mknob::var_number($vid) \
                [string map {"$" {\$}} [unspace_text $sendname]] \
                [string map {"$" {\$}} [unspace_text $receivename]] \
                $labelname \
                $::dialog_mknob::var_label_dx($vid) \
                $::dialog_mknob::var_label_dy($vid) \
                $::dialog_mknob::var_label_font($vid) \
                $::dialog_mknob::var_label_fontsize($vid) \
                [string tolower $::dialog_mknob::var_color_background($vid)] \
                [string tolower $::dialog_mknob::var_color_foreground($vid)] \
                [string tolower $::dialog_mknob::var_color_label($vid)] \
                $::dialog_mknob::var_steady($vid) \
               ]

    foreach v ${fallbacks} {
        array set ::dialog_mknob::old_${v} [array get ::dialog_mknob::var_${v} ${vid}]
    }
}


proc ::dialog_mknob::cancel {mytoplevel} {
    pdsend [list $mytoplevel cancel]

    set vid [string trimleft $mytoplevel .]
    foreach v [info vars ::dialog_mknob::*] {
        if { [array exists $v] } {
            if { [array get $v ${vid}] ne {} } {
                array unset $v ${vid}
            }
        }
    }
}

proc ::dialog_mknob::ok {mytoplevel} {
    ::dialog_mknob::apply $mytoplevel
    ::dialog_mknob::cancel $mytoplevel
}

proc ::dialog_mknob::bind_capslock {tag seq_prefix seq_nocase script} {
    bind $tag <${seq_prefix}-[string tolower ${seq_nocase}]> "::pd_menucommands::scheduleAction $script"
    bind $tag <${seq_prefix}-[string toupper ${seq_nocase}]> "::pd_menucommands::scheduleAction $script"
}

proc ::dialog_mknob::mknob_properties {mytoplevel mainheader dim_header_UNUSED \
                                       wdt min_wdt label_width \
                                       hgt min_hgt label_height \
                                       label_range min_rng label_range_min max_rng \
                                       label_range_max rng_sched \
                                       lin0_log1 lilo0_label lilo1_label \
                                       loadbang steady label_number num \
                                       snd rcv \
                                       gui_name \
                                       gn_dx gn_dy gn_f gn_fs \
                                       bcol fcol lcol} {

    set vid [string trimleft $mytoplevel .]
    set snd [::pdtk_text::unescape $snd]
    set rcv [::pdtk_text::unescape $rcv]
    set gui_name [::pdtk_text::unescape $gui_name]

    # initialize the array
    set ::dialog_mknob::var_width($vid) $wdt
    set ::dialog_mknob::var_height($vid) $hgt
    set ::dialog_mknob::var_minwidth($vid) $min_wdt
    set ::dialog_mknob::var_minheight($vid) $min_hgt

    set ::dialog_mknob::var_range_max($vid) $max_rng
    set ::dialog_mknob::var_range_min($vid) $min_rng
    set ::dialog_mknob::var_range_checkmode($vid) $rng_sched

    set ::dialog_mknob::var_mode($vid) $lin0_log1
    set ::dialog_mknob::var_loadbang($vid) $loadbang
    set ::dialog_mknob::var_steady($vid) $steady
    set ::dialog_mknob::var_number($vid) $num

    set ::dialog_mknob::var_snd($vid) $snd
    set ::dialog_mknob::var_rcv($vid) $rcv
    set ::dialog_mknob::var_label($vid) $gui_name

    set ::dialog_mknob::var_label_dx($vid) $gn_dx
    set ::dialog_mknob::var_label_dy($vid) $gn_dy
    set ::dialog_mknob::var_label_font($vid) $gn_f
    set ::dialog_mknob::var_label_fontsize($vid) $gn_fs

    set ::dialog_mknob::var_color_background($vid) $bcol
    set ::dialog_mknob::var_color_foreground($vid) $fcol
    set ::dialog_mknob::var_color_label($vid) $lcol
    set ::dialog_mknob::var_colortype($vid) 0

    # fallback values (in case the user enters garbage)
    set ::dialog_mknob::old_width($vid) $wdt
    set ::dialog_mknob::old_height($vid) $hgt
    set ::dialog_mknob::old_range_max($vid) $max_rng
    set ::dialog_mknob::old_range_min($vid) $min_rng
    set ::dialog_mknob::old_number($vid) $num
    set ::dialog_mknob::old_label_dx($vid) $gn_dx
    set ::dialog_mknob::old_label_dy($vid) $gn_dy
    set ::dialog_mknob::old_label_font($vid) $gn_f
    set ::dialog_mknob::old_label_fontsize($vid) $gn_fs

    set iemgui_type [_ $mainheader]

    switch -- $mainheader {
        "Mknob" {
            set iemgui_type [_ "Mknob"]
            set label_width [_ "Size:"]
            set label_height [_ "Height:"]
            set label_range [_ "Output Range"]
            set label_range_min [_ "Lower:"]
            set label_range_max [_ "Upper:"] }
    }

    toplevel $mytoplevel -class DialogWindow
    #wm title $mytoplevel [_ "%s Properties" $iemgui_type] # only available from Pd 0.55(?)
    wm title $mytoplevel [string cat [_ "Properties"] " | $iemgui_type"]
    wm group $mytoplevel .
    wm resizable $mytoplevel 0 0
    wm transient $mytoplevel $::focused_window
    #::pd_menus::menubar_for_dialog $mytoplevel # seems useless, and not working before Pd 0.55(?)
    $mytoplevel configure -padx 0 -pady 0

    # bindings
    bind $mytoplevel <KeyPress-Escape>          "dialog_mknob::cancel $mytoplevel; break"
    bind $mytoplevel <KeyPress-Return>          "dialog_mknob::ok $mytoplevel; break"
    bind_capslock $mytoplevel $::modifier-Key w "dialog_mknob::cancel $mytoplevel; break"
      # these aren't supported in the dialog, so alert the user
    bind_capslock $mytoplevel $::modifier-Key s       {bell; break}
    bind_capslock $mytoplevel $::modifier-Shift-Key s {bell; break}
    bind_capslock $mytoplevel $::modifier-Key p       {bell; break}
    bind_capslock $mytoplevel $::modifier-Key t       {bell; break}
    wm protocol $mytoplevel WM_DELETE_WINDOW "dialog_mknob::cancel $mytoplevel"


    # dimensions
    frame $mytoplevel.dim -height 7
    pack $mytoplevel.dim -side top
    label $mytoplevel.dim.w_lab -text [_ $label_width]
    entry $mytoplevel.dim.w_ent -textvariable ::dialog_mknob::var_width($vid) -width 4
    label $mytoplevel.dim.dummy1 -text "" -width 1
    label $mytoplevel.dim.h_lab -text [_ $label_height]
    entry $mytoplevel.dim.h_ent -textvariable ::dialog_mknob::var_height($vid) -width 4
    pack $mytoplevel.dim.w_lab $mytoplevel.dim.w_ent -side left
    if { $label_height ne "" } {
        pack $mytoplevel.dim.dummy1 $mytoplevel.dim.h_lab $mytoplevel.dim.h_ent -side left }

    # range
    labelframe $mytoplevel.rng
    pack $mytoplevel.rng -side top -fill x
    frame $mytoplevel.rng.min
    label $mytoplevel.rng.min.lab -text $label_range_min
    entry $mytoplevel.rng.min.ent -textvariable ::dialog_mknob::var_range_min($vid) -width 7
    label $mytoplevel.rng.dummy1 -text "" -width 1
    label $mytoplevel.rng.max_lab -text [_ $label_range_max]
    entry $mytoplevel.rng.max_ent -textvariable ::dialog_mknob::var_range_max($vid) -width 7
    if { $label_range ne "" } {
        $mytoplevel.rng config -borderwidth 1 -pady 4 -text [_ $label_range]
        if { $label_range_min ne "" } {
            pack $mytoplevel.rng.min
            pack $mytoplevel.rng.min.lab $mytoplevel.rng.min.ent -side left }
        if { $label_range_max ne "" } {
            $mytoplevel.rng config -padx 26
            pack configure $mytoplevel.rng.min -side left
            pack $mytoplevel.rng.dummy1 $mytoplevel.rng.max_lab $mytoplevel.rng.max_ent -side left}
    }

    # parameters
    labelframe $mytoplevel.para -borderwidth 1 -padx 5 -pady 5 -text [_ "Parameters"]
    pack $mytoplevel.para -side top -fill x -pady 5

    frame $mytoplevel.para.num
    label $mytoplevel.para.num.lab -text [_ $label_number]
    entry $mytoplevel.para.num.ent -textvariable ::dialog_mknob::var_number($vid) -width 4
    pack $mytoplevel.para.num.ent $mytoplevel.para.num.lab -side right -anchor e

    set applycmd ""
    if {$::windowingsystem eq "aqua"} {
        set applycmd "::dialog_mknob::apply $mytoplevel"
    }


    if {$::dialog_mknob::var_mode($vid) >= 0} {
        if {$mainheader == "|nbx|" } {
            set togglecmd "::dialog_mknob::toggle_and_activate $mytoplevel $mytoplevel.para.num.ent"
        } else {
            set togglecmd "::dialog_mknob::toggle_mode $mytoplevel"
        }
        ::dialog_mknob::popupmenu $mytoplevel.para.lilo \
            ::dialog_mknob::var_mode($vid) [list [_ $lilo0_label] [_ $lilo1_label] ] \
            "$togglecmd; $applycmd"
        pack $mytoplevel.para.lilo -side left -expand 1 -ipadx 10
    }
    if {$::dialog_mknob::var_loadbang($vid) >= 0} {
        ::dialog_mknob::popupmenu $mytoplevel.para.lb \
            ::dialog_mknob::var_loadbang($vid) [list [_ "No init"] [_ "Init"] ] \
            $applycmd
        pack $mytoplevel.para.lb -side left -expand 1 -ipadx 10
    }
    if {$::dialog_mknob::var_number($vid) > 0} {
        pack $mytoplevel.para.num -side left -expand 1 -ipadx 10
    }
    if {$::dialog_mknob::var_steady($vid) >= 0} {
        ::dialog_mknob::popupmenu $mytoplevel.para.stdy_jmp \
            ::dialog_mknob::var_steady($vid) [list [_ "Jump on click"] [_ "Steady on click"] ] \
            $applycmd
        pack $mytoplevel.para.stdy_jmp -side left -expand 1 -ipadx 10
    }

    # messages
    labelframe $mytoplevel.s_r -borderwidth 1 -padx 5 -pady 5 -text [_ "Messages"]
    pack $mytoplevel.s_r -side top -fill x
    frame $mytoplevel.s_r.send
    pack $mytoplevel.s_r.send -side top -anchor e -padx 5
    label $mytoplevel.s_r.send.lab -text [_ "Send symbol:"]
    entry $mytoplevel.s_r.send.ent -textvariable ::dialog_mknob::var_snd($vid) -width 21
    if { $snd ne "nosndno" } {
        pack $mytoplevel.s_r.send.lab $mytoplevel.s_r.send.ent -side left \
            -fill x -expand 1
    }

    frame $mytoplevel.s_r.receive
    pack $mytoplevel.s_r.receive -side top -anchor e -padx 5
    label $mytoplevel.s_r.receive.lab -text [_ "Receive symbol:"]
    entry $mytoplevel.s_r.receive.ent -textvariable ::dialog_mknob::var_rcv($vid) -width 21
    if { $rcv ne "norcvno" } {
        pack $mytoplevel.s_r.receive.lab $mytoplevel.s_r.receive.ent -side left \
            -fill x -expand 1
    }

    # get the current font name from the int given from C-space (gn_f)
    set current_font $::font_family
    if {$::dialog_mknob::var_label_font($vid) == 1} \
        { set current_font "Helvetica" }
    if {$::dialog_mknob::var_label_font($vid) == 2} \
        { set current_font "Times" }

    # label
    labelframe $mytoplevel.label -borderwidth 1 -text [_ "Label"] -padx 5 -pady 5
    pack $mytoplevel.label -side top -fill x -pady 5
    entry $mytoplevel.label.name_entry -textvariable ::dialog_mknob::var_label($vid) \
        -width 30 -font [list $current_font 14 $::font_weight]
    pack $mytoplevel.label.name_entry -side top -fill both -padx 5

    frame $mytoplevel.label.xy -padx 20 -pady 1
    pack $mytoplevel.label.xy -side top
    label $mytoplevel.label.xy.x_lab -text [_ "X offset:"]
    entry $mytoplevel.label.xy.x_entry -textvariable ::dialog_mknob::var_label_dx($vid) -width 5
    label $mytoplevel.label.xy.dummy1 -text " " -width 1
    label $mytoplevel.label.xy.y_lab -text [_ "Y offset:"]
    entry $mytoplevel.label.xy.y_entry -textvariable ::dialog_mknob::var_label_dy($vid) -width 5
    pack $mytoplevel.label.xy.x_lab $mytoplevel.label.xy.x_entry $mytoplevel.label.xy.dummy1 \
        $mytoplevel.label.xy.y_lab $mytoplevel.label.xy.y_entry -side left

    button $mytoplevel.label.fontpopup_label -text $current_font \
        -font [list $current_font 16 $::font_weight] -pady 4 \
        -command "::dialog_mknob::font_popup $mytoplevel"
    pack $mytoplevel.label.fontpopup_label -side left -anchor w \
        -expand 1 -fill x -padx 5
    frame $mytoplevel.label.fontsize
    pack $mytoplevel.label.fontsize -side right -padx 5 -pady 5
    label $mytoplevel.label.fontsize.label -text [_ "Size:"]
    entry $mytoplevel.label.fontsize.entry -textvariable ::dialog_mknob::var_label_fontsize($vid) -width 4
    pack $mytoplevel.label.fontsize.entry $mytoplevel.label.fontsize.label \
        -side right -anchor e
    menu $mytoplevel.popup
    $mytoplevel.popup add command \
        -label $::font_family \
        -font [format {{%s} 16 %s} $::font_family $::font_weight] \
        -command "::dialog_mknob::toggle_font $mytoplevel 0"
    $mytoplevel.popup add command \
        -label "Helvetica" \
        -font [format {Helvetica 16 %s} $::font_weight] \
        -command "::dialog_mknob::toggle_font $mytoplevel 1"
    $mytoplevel.popup add command \
        -label "Times" \
        -font [format {Times 16 %s} $::font_weight] \
        -command "::dialog_mknob::toggle_font $mytoplevel 2"

    # colors
    labelframe $mytoplevel.colors -borderwidth 1 -text [_ "Colors"] -padx 5 -pady 5
    pack $mytoplevel.colors -fill x

    frame $mytoplevel.colors.select
    pack $mytoplevel.colors.select -side top
    radiobutton $mytoplevel.colors.select.radio0 \
        -value 0 -variable ::dialog_mknob::var_colortype($vid) \
        -text [_ "Background"] -justify left
    radiobutton $mytoplevel.colors.select.radio1 \
        -value 1 -variable ::dialog_mknob::var_colortype($vid) \
        -text [_ "Front"] -justify left
    radiobutton $mytoplevel.colors.select.radio2 \
        -value 2 -variable ::dialog_mknob::var_colortype($vid) \
        -text [_ "Label"] -justify left
    if { $::dialog_mknob::var_color_foreground($vid) ne "none" } {
        pack $mytoplevel.colors.select.radio0 $mytoplevel.colors.select.radio1 \
            $mytoplevel.colors.select.radio2 -side left
    } else {
        pack $mytoplevel.colors.select.radio0 $mytoplevel.colors.select.radio2 -side left
    }

    frame $mytoplevel.colors.sections
    pack $mytoplevel.colors.sections -side top
    button $mytoplevel.colors.sections.but -text [_ "Compose color"] \
        -command "::dialog_mknob::choose_col_bkfrlb $mytoplevel"
    pack $mytoplevel.colors.sections.but -side left -anchor w -pady 5 \
        -expand yes -fill x
    frame $mytoplevel.colors.sections.exp
    pack $mytoplevel.colors.sections.exp -side right -padx 5
    if { $::dialog_mknob::var_color_foreground($vid) ne "none" } {
        label $mytoplevel.colors.sections.exp.fr_bk -text "o=||=o" -width 6 \
            -background $::dialog_mknob::var_color_background($vid) \
            -activebackground $::dialog_mknob::var_color_background($vid) \
            -foreground $::dialog_mknob::var_color_foreground($vid) \
            -activeforeground $::dialog_mknob::var_color_foreground($vid) \
            -font [list $current_font 14 $::font_weight] -padx 2 -pady 2 -relief ridge
    } else {
        label $mytoplevel.colors.sections.exp.fr_bk -text "o=||=o" -width 6 \
            -background $::dialog_mknob::var_color_background($vid) \
            -activebackground $::dialog_mknob::var_color_background($vid) \
            -foreground $::dialog_mknob::var_color_background($vid) \
            -activeforeground $::dialog_mknob::var_color_background($vid) \
            -font [list $current_font 14 $::font_weight] -padx 2 -pady 2 -relief ridge
    }
    label $mytoplevel.colors.sections.exp.lb_bk -text [_ "Test label"] \
        -background $::dialog_mknob::var_color_background($vid) \
        -activebackground $::dialog_mknob::var_color_background($vid) \
        -foreground $::dialog_mknob::var_color_label($vid) \
        -activeforeground $::dialog_mknob::var_color_label($vid) \
        -font [list $current_font 14 $::font_weight] -padx 2 -pady 2 -relief ridge
    pack $mytoplevel.colors.sections.exp.lb_bk $mytoplevel.colors.sections.exp.fr_bk \
        -side right -anchor e -expand yes -fill both -pady 7

    # color scheme by Mary Ann Benedetto http://piR2.org
    foreach r {r1 r2 r3} hexcols {
       { "#FFFFFF" "#DFDFDF" "#BBBBBB" "#FFC7C6" "#FFE3C6" "#FEFFC6" "#C6FFC7" "#C6FEFF" "#C7C6FF" "#E3C6FF" }
       { "#9F9F9F" "#7C7C7C" "#606060" "#FF0400" "#FF8300" "#FAFF00" "#00FF04" "#00FAFF" "#0400FF" "#9C00FF" }
       { "#404040" "#202020" "#000000" "#551312" "#553512" "#535512" "#0F4710" "#0E4345" "#131255" "#2F004D" } } \
    {
       frame $mytoplevel.colors.$r
       pack $mytoplevel.colors.$r -side top
       foreach i { 0 1 2 3 4 5 6 7 8 9} hexcol $hexcols \
           {
               label $mytoplevel.colors.$r.c$i -background $hexcol -activebackground $hexcol -relief ridge -padx 7 -pady 0 -width 1
               bind $mytoplevel.colors.$r.c$i <Button> "::dialog_mknob::preset_col $mytoplevel $hexcol"
           }
       pack $mytoplevel.colors.$r.c0 $mytoplevel.colors.$r.c1 $mytoplevel.colors.$r.c2 $mytoplevel.colors.$r.c3 \
           $mytoplevel.colors.$r.c4 $mytoplevel.colors.$r.c5 $mytoplevel.colors.$r.c6 $mytoplevel.colors.$r.c7 \
           $mytoplevel.colors.$r.c8 $mytoplevel.colors.$r.c9 -side left
    }

    # buttons
    frame $mytoplevel.cao -pady 10
    pack $mytoplevel.cao -side top
    button $mytoplevel.cao.cancel -text [_ "Cancel"] \
        -command "::dialog_mknob::cancel $mytoplevel"
    pack $mytoplevel.cao.cancel -side left -expand 1 -fill x -padx 15 -ipadx 10
    if {$::windowingsystem ne "aqua"} {
        button $mytoplevel.cao.apply -text [_ "Apply"] \
            -command "::dialog_mknob::apply $mytoplevel"
        pack $mytoplevel.cao.apply -side left -expand 1 -fill x -padx 15 -ipadx 10
    }
    button $mytoplevel.cao.ok -text [_ "OK"] \
        -command "::dialog_mknob::ok $mytoplevel" -default active
    pack $mytoplevel.cao.ok -side left -expand 1 -fill x -padx 15 -ipadx 10

    $mytoplevel.dim.w_ent select from 0
    $mytoplevel.dim.w_ent select adjust end
    focus $mytoplevel.dim.w_ent

    # live widget updates on OSX in lieu of Apply button
    if {$::windowingsystem eq "aqua"} {

        # call apply on Return in entry boxes that are in focus & rebind Return to ok button
        bind $mytoplevel.dim.w_ent <KeyPress-Return> "::dialog_mknob::apply_and_rebind_return $mytoplevel"
        bind $mytoplevel.dim.h_ent <KeyPress-Return> "::dialog_mknob::apply_and_rebind_return $mytoplevel"
        bind $mytoplevel.rng.min.ent <KeyPress-Return> "::dialog_mknob::apply_and_rebind_return $mytoplevel"
        bind $mytoplevel.rng.max_ent <KeyPress-Return> "::dialog_mknob::apply_and_rebind_return $mytoplevel"
        bind $mytoplevel.para.num.ent <KeyPress-Return> "::dialog_mknob::apply_and_rebind_return $mytoplevel"
        bind $mytoplevel.label.name_entry <KeyPress-Return> "::dialog_mknob::apply_and_rebind_return $mytoplevel"
        bind $mytoplevel.s_r.send.ent <KeyPress-Return> "::dialog_mknob::apply_and_rebind_return $mytoplevel"
        bind $mytoplevel.s_r.receive.ent <KeyPress-Return> "::dialog_mknob::apply_and_rebind_return $mytoplevel"
        bind $mytoplevel.label.xy.x_entry <KeyPress-Return> "::dialog_mknob::apply_and_rebind_return $mytoplevel"
        bind $mytoplevel.label.xy.y_entry <KeyPress-Return> "::dialog_mknob::apply_and_rebind_return $mytoplevel"
        bind $mytoplevel.label.fontsize.entry <KeyPress-Return> "::dialog_mknob::apply_and_rebind_return $mytoplevel"

        # unbind Return from ok button when an entry takes focus
        $mytoplevel.dim.w_ent config -validate focusin -vcmd "::dialog_mknob::unbind_return $mytoplevel"
        $mytoplevel.dim.h_ent config -validate focusin -vcmd "::dialog_mknob::unbind_return $mytoplevel"
        $mytoplevel.rng.min.ent config -validate focusin -vcmd "::dialog_mknob::unbind_return $mytoplevel"
        $mytoplevel.rng.max_ent config -validate focusin -vcmd "::dialog_mknob::unbind_return $mytoplevel"
        $mytoplevel.para.num.ent config -validate focusin -vcmd "::dialog_mknob::unbind_return $mytoplevel"
        $mytoplevel.label.name_entry config -validate focusin -vcmd "::dialog_mknob::unbind_return $mytoplevel"
        $mytoplevel.s_r.send.ent config -validate focusin -vcmd "::dialog_mknob::unbind_return $mytoplevel"
        $mytoplevel.s_r.receive.ent config -validate focusin -vcmd "::dialog_mknob::unbind_return $mytoplevel"
        $mytoplevel.label.xy.x_entry config -validate focusin -vcmd "::dialog_mknob::unbind_return $mytoplevel"
        $mytoplevel.label.xy.y_entry config -validate focusin -vcmd "::dialog_mknob::unbind_return $mytoplevel"
        $mytoplevel.label.fontsize.entry config -validate focusin -vcmd "::dialog_mknob::unbind_return $mytoplevel"

        # remove cancel button from focus list since it's not activated on Return
        $mytoplevel.cao.cancel config -takefocus 0

        # show active focus on the ok button as it *is* activated on Return
        $mytoplevel.cao.ok config -default normal
        bind $mytoplevel.cao.ok <FocusIn> "$mytoplevel.cao.ok config -default active"
        bind $mytoplevel.cao.ok <FocusOut> "$mytoplevel.cao.ok config -default normal"

        # since we show the active focus, disable the highlight outline
        $mytoplevel.cao.ok config -highlightthickness 0
        $mytoplevel.cao.cancel config -highlightthickness 0
    }

    position_over_window $mytoplevel $::focused_window

    if {$mainheader == "|nbx|" } {
        ::dialog_mknob::toggle_and_activate $mytoplevel $mytoplevel.para.num.ent
    }
}

# for live widget updates on OSX
proc ::dialog_mknob::apply_and_rebind_return {mytoplevel} {
    ::dialog_mknob::apply $mytoplevel
    bind $mytoplevel <KeyPress-Return> "::dialog_mknob::ok $mytoplevel"
    focus $mytoplevel.cao.ok
    return 0
}

# for live widget updates on OSX
proc ::dialog_mknob::unbind_return {mytoplevel} {
    bind $mytoplevel <KeyPress-Return> break
    return 1
}
