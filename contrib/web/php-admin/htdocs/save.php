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
    
    $file = $topdir."/".$list."/control/".$name;
    
    if(isset($_POST[$name]) && !empty($_POST[$name]))
    {
       if(!touch($file))
          die("Couldn't open ".$file." for writing");
       // don't error on chmod, if the owner of the file is different it won't work
       @chmod($file, 0664);
    }
    else {
       if (file_exists($file)) {
          if (!unlink($file))
             die("Couldn't unlink ".$file);
       }
    }
}

function mlmmj_string ($name, $nicename, $text) 
{
    mlmmj_list($name, $nicename, $text);
}

function mlmmj_list($name, $nicename, $text) 
{
    global $tpl, $topdir, $list;

    $file = $topdir."/".$list."/control/".$name;
    
    if(isset($_POST[$name]) && !empty($_POST[$name]) && !preg_match('/^\s*$/',$_POST[$name]))
    {
       // remove all \r
       $_POST[$name]=preg_replace('/\r/',"",$_POST[$name]);

       // no trailing \n?, then we add one
       if (!preg_match('/\n$/',$_POST[$name]))
          $_POST[$name].="\n";

       // we don't like whitespace before a \n
       $_POST[$name]=preg_replace('/\s*\n/',"\n",$_POST[$name]);

       if (!$fp = fopen($file, "w"))
          die("Couldn't open ".$file." for writing");

       // write the result in a file
       fwrite($fp, $_POST[$name]);
       fclose($fp);

       // don't error on chmod, if the owner of the file is different it won't work
       @chmod($file, 0664);
    }
    else {
       if (file_exists($file)) {
          if (!unlink($file))
             die("Couldn't unlink ".$file);
       }
    }
}

// Perl's encode_entities (to be able to use tunables.pl)
function encode_entities($str) { return htmlentities($str); }


$tpl = new rFastTemplate($templatedir);

if(empty($_POST['list']))
        die("no list specified");
$list = $_POST['list'];

if (dirname(realpath($topdir."/".$list)) != realpath($topdir))
die("list outside topdir");

if(!is_dir($topdir."/".$list))
die("non-existent list");

$userfile = "$topdir/$list/control/admin_users";
$admin_user = "";
if(is_file($userfile)) {
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

$tpl->define(array("main" => "save.html"));
$tpl->assign(array("LIST" => htmlentities($list)));

$tunables = file_get_contents($confdir.'/tunables.pl');
eval($tunables);

$tpl->parse("MAIN","main");
$tpl->FastPrint("MAIN");

?>
