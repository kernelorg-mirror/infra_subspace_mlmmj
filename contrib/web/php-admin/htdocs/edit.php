<?php

/* mlmmj/php-admin:
 * Copyright (C) 2023 Franky Van Liedekerke <franky at e-dynamics dot be>
 * Copyright (C) 2004 Christoph Thiel <ct at kki dot org>
 *
 * mlmmj/php-perl:
 * Copyright (C) 2004 Morten K. Poulsen <morten at afdelingp.dk>
 * Copyright (C) 2004 Christian Laursen <christian@pil.dk>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

require(dirname(dirname(__FILE__))."/conf/config.php");
require(dirname(__FILE__)."/class.rFastTemplate.php");

function mlmmj_boolean($name, $nicename, $text) 
{
    global $tpl, $topdir, $list;

    if(is_file($topdir."/".$list."/control/".$name))
       $checked = TRUE;
    else
       $checked = FALSE;

    $tpl->assign(array("NAME" => htmlentities($name),
		       "NICENAME" => htmlentities($nicename),
		       "TEXT" => nl2br(htmlentities($text))));
    $tpl->assign(array("CHECKED" => $checked ? " checked" : ""));

    $tpl->parse("ROWS",".boolean");
}

function mlmmj_string($name, $nicename, $text)
{
    global $tpl, $topdir, $list;

    $file = $topdir."/".$list."/control/".$name;
    $value = "";

    if(is_file($file)) {
       $lines = file($file);
       $value = $lines[0];
    }

    // remove trailing \n if any, just to be sure
    $value = preg_replace('/\n$/',"",$value);
    
    $tpl->assign(array("NAME" => htmlentities($name),
		       "NICENAME" => htmlentities($nicename),
		       "TEXT" => nl2br(htmlentities($text)),
		       "VALUE" => htmlentities($value)));
    
    $tpl->parse("ROWS",".string");
}

function mlmmj_list($name, $nicename, $text)
{
    global $tpl, $topdir, $list;
    
    $file = "$topdir/$list/control/$name";
    $value = "";

    if(is_file($file))
       $value = file_get_contents($file);

    // the last \n would result in an extra empty line in the list box,
    // so we remove it
    $value = preg_replace('/\n$/',"",$value);

    $tpl->assign(array("NAME" => htmlentities($name),
		       "NICENAME" => htmlentities($nicename),
		       "TEXT" => nl2br(htmlentities($text)),
		       "VALUE" => htmlentities($value)));

    $tpl->parse("ROWS",".list");
}

// Perl's encode_entities (to be able to use tunables.pl)
function encode_entities($str) { return htmlentities($str); } 


$tpl = new rFastTemplate($templatedir);

if(empty($_GET['list']))
	die("no list specified");
$list = $_GET['list'];

if (dirname(realpath($topdir."/".$list)) != realpath($topdir))
die("list outside topdir");

if(!is_dir($topdir."/".$list))
die("non-existent list");

$userfile = "$topdir/$list/control/admin_users";
if (is_file($userfile)) {
	// read users into array
        $admin_users = array_map('trim',file($userfile));
	// remove empty values from array
	$admin_users = array_filter($admin_users);
        if (!isset($_SERVER['PHP_AUTH_USER']) || !in_array($_SERVER['PHP_AUTH_USER'],$admin_users)) {
                header("WWW-Authenticate: " .
                        "Basic realm=\"Mlmmj Protected Area\"");
                header("HTTP/1.0 401 Unauthorized");
                //Show failure text, which browsers usually
                //show only after several failed attempts
                print("This page is protected by HTTP " .
                        "Authentication.<br>\n");
                exit;
        }
}

$tpl->define(array("main" => "edit.html",
		   "boolean" => "edit_boolean.html",
		   "string" => "edit_string.html",
		   "list" => "edit_list.html"));

$tpl->assign(array("LIST" =>htmlentities($list)));

$tunables = file_get_contents($confdir.'/tunables.pl');
eval($tunables);

$tpl->parse("MAIN","main");
$tpl->FastPrint("MAIN");

?>
