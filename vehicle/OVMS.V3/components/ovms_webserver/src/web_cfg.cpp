/*
;    Project:       Open Vehicle Monitor System
;    Date:          14th March 2017
;
;    Changes:
;    1.0  Initial release
;
;    (C) 2018       Michael Balzer
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
; THE SOFTWARE.
*/

// #include "ovms_log.h"
// static const char *TAG = "webserver";

#include <string.h>
#include <stdio.h>
#include "ovms_webserver.h"
#include "ovms_config.h"
#include "ovms_metrics.h"
#include "metrics_standard.h"
#include "vehicle.h"

#define _attr(text) (c.encode_html(text).c_str())
#define _html(text) (c.encode_html(text).c_str())


/**
 * HandleStatus: show status overview
 */
void OvmsWebServer::HandleStatus(PageEntry_t& p, PageContext_t& c)
{
  std::string cmd, output;
  
  c.head(200);
  
  if ((cmd = c.getvar("cmd")) != "") {
    output = ExecuteCommand(cmd);
    output =
      "<samp>" + c.encode_html(output) + "</samp>" +
      "<p><a class=\"btn btn-default\" target=\"#main\" href=\"/status\">Reload status</a></p>";
    c.alert("info", output.c_str());
  }
  
  c.printf("<div class=\"row\"><div class=\"col-md-6\">");
  
  c.panel_start("primary", "Vehicle Status");
  output = ExecuteCommand("stat");
  c.printf("<samp class=\"monitor\" id=\"vehicle-status\" data-updcmd=\"stat\">%s</samp>", _html(output));
  output = ExecuteCommand("location status");
  c.printf("<samp>%s</samp>", _html(output));
  c.panel_end(
    "<ul class=\"list-inline\">"
      "<li><button type=\"button\" class=\"btn btn-default btn-sm\" data-target=\"#vehicle-status\" data-cmd=\"charge start\">Start charge</button></li>"
      "<li><button type=\"button\" class=\"btn btn-default btn-sm\" data-target=\"#vehicle-status\" data-cmd=\"charge stop\">Stop charge</button></li>"
    "</ul>");
  
  c.printf("</div><div class=\"col-md-6\">");

  c.panel_start("primary", "Server Status");
  output = ExecuteCommand("server v2 status");
  if (!startsWith(output, "Unrecognised"))
    c.printf("<samp class=\"monitor\" id=\"server-v2\" data-updcmd=\"server v2 status\">%s</samp>", _html(output));
  output = ExecuteCommand("server v3 status");
  if (!startsWith(output, "Unrecognised"))
    c.printf("<samp class=\"monitor\" id=\"server-v3\" data-updcmd=\"server v3 status\">%s</samp>", _html(output));
  c.panel_end(
    "<ul class=\"list-inline\">"
      "<li><button type=\"button\" class=\"btn btn-default btn-sm\" data-target=\"#server-v2\" data-cmd=\"server v2 start\">Start V2</button></li>"
      "<li><button type=\"button\" class=\"btn btn-default btn-sm\" data-target=\"#server-v2\" data-cmd=\"server v2 stop\">Stop V2</button></li>"
      "<li><button type=\"button\" class=\"btn btn-default btn-sm\" data-target=\"#server-v3\" data-cmd=\"server v3 start\">Start V3</button></li>"
      "<li><button type=\"button\" class=\"btn btn-default btn-sm\" data-target=\"#server-v3\" data-cmd=\"server v3 stop\">Stop V3</button></li>"
    "</ul>");
  
  c.printf("</div></div>");
  c.printf("<div class=\"row\"><div class=\"col-md-6\">");
  
  c.panel_start("primary", "Wifi Status");
  output = ExecuteCommand("wifi status");
  c.printf("<samp>%s</samp>", _html(output));
  c.panel_end();
  
  c.printf("</div><div class=\"col-md-6\">");
  
  c.panel_start("primary", "Modem Status");
  output = ExecuteCommand("simcom status");
  c.printf("<samp>%s</samp>", _html(output));
  c.panel_end();
  
  c.printf("</div></div>");
  c.printf("<div class=\"row\"><div class=\"col-md-12\">");
  
  c.panel_start("primary", "Firmware Status");
  output = ExecuteCommand("ota status");
  c.printf("<samp>%s</samp>", _html(output));
  c.panel_end();
  
  c.printf("</div></div");
  
  c.done();
}


/**
 * HandleCommand: execute command, send output
 *  Default: plain text
 *  HTML encoded with parameter encode=html
 */
void OvmsWebServer::HandleCommand(PageEntry_t& p, PageContext_t& c)
{
  std::string command = c.getvar("command");
  std::string output = ExecuteCommand(command);
  if (c.getvar("encode") == "html") {
    c.head(200,
      "Content-Type: text/html; charset=utf-8\r\n"
      "Cache-Control: no-cache");
    c.print(c.encode_html(output));
  }
  else {
    c.head(200,
      "Content-Type: text/plain; charset=utf-8\r\n"
      "Cache-Control: no-cache");
    c.print(output);
  }
  c.done();
}


/**
 * HandleShell: command shell
 */
void OvmsWebServer::HandleShell(PageEntry_t& p, PageContext_t& c)
{
  std::string command = c.getvar("command");
  std::string output;
  
  if (command != "")
    output = ExecuteCommand(command);

  // generate form:
  c.head(200);
  c.panel_start("primary", "Shell");
  
  c.printf(
    "<pre class=\"get-window-resize\" id=\"output\">%s</pre>"
    "<form id=\"shellform\" method=\"post\" action=\"#\">"
      "<div class=\"input-group\">"
        "<label class=\"input-group-addon hidden-xs\" for=\"input-command\">OVMS&nbsp;&gt;&nbsp;</label>"
        "<input type=\"text\" class=\"form-control font-monospace\" placeholder=\"Enter command\" name=\"command\" id=\"input-command\" value=\"%s\">"
        "<div class=\"input-group-btn\">"
          "<button type=\"submit\" class=\"btn btn-default\">Execute</button>"
        "</div>"
      "</div>"
    "</form>"
    "<script>"
    "$(\"#output\").on(\"window-resize\", function(){"
      "$(\"#output\").height($(window).height()-220);"
      "$(\"#output\").scrollTop($(\"#output\").get(0).scrollHeight);"
    "}).trigger(\"window-resize\");"
    "$(\"#shellform\").on(\"submit\", function(){"
      "var data = $(this).serialize();"
      "var command = $(\"#input-command\").val();"
      "var output = $(\"#output\");"
      "$.ajax({ \"type\": \"post\", \"url\": \"/api/execute\", \"data\": data,"
        "\"timeout\": 5000,"
        "\"beforeSend\": function(){"
          "$(\"html\").addClass(\"loading\");"
          "output.html(output.html() + \"<strong>OVMS&nbsp;&gt;&nbsp;</strong><kbd>\""
            "+ $(\"<div/>\").text(command).html() + \"</kbd><br>\");"
          "output.scrollTop(output.get(0).scrollHeight);"
        "},"
        "\"complete\": function(){"
          "$(\"html\").removeClass(\"loading\");"
        "},"
        "\"success\": function(response){"
          "output.html(output.html() + $(\"<div/>\").text(response).html());"
          "output.scrollTop(output.get(0).scrollHeight);"
        "},"
        "\"error\": function(response, status, httperror){"
          "var resptext = response.responseText || httperror || status;"
          "output.html(output.html() + $(\"<div/>\").text(resptext).html());"
          "output.scrollTop(output.get(0).scrollHeight);"
        "},"
      "});"
      "event.stopPropagation();"
      "return false;"
    "});"
    "$(\"#input-command\").focus();"
    "</script>"
    , _html(output.c_str()), _attr(command.c_str()));
  
  c.panel_end();
  c.done();
}


/**
 * HandleCfgPassword: change admin password
 */
void OvmsWebServer::HandleCfgPassword(PageEntry_t& p, PageContext_t& c)
{
  std::string error, info;
  std::string oldpass, newpass1, newpass2;
  
  if (c.method == "POST") {
    // process form submission:
    oldpass = c.getvar("oldpass");
    newpass1 = c.getvar("newpass1");
    newpass2 = c.getvar("newpass2");
    
    if (oldpass != MyConfig.GetParamValue("password", "module"))
      error += "<li data-input=\"oldpass\">Old password is not correct</li>";
    if (newpass1 == "")
      error += "<li data-input=\"newpass1\">New password may not be empty</li>";
    if (newpass2 != newpass1)
      error += "<li data-input=\"newpass2\">Passwords do not match</li>";
    
    if (error == "") {
      // success:
      MyConfig.SetParamValue("password", "module", newpass1);
      info += "<li>New module &amp; admin password has been set.</li>";
      
      info = "<p class=\"lead\">Success!</p><ul class=\"infolist\">" + info + "</ul>";
      c.head(200);
      c.alert("success", info.c_str());
      c.done();
      return;
    }
    
    // output error, return to form:
    error = "<p class=\"lead\">Error!</p><ul class=\"errorlist\">" + error + "</ul>";
    c.head(400);
    c.alert("danger", error.c_str());
  }
  else {
    c.head(200);
  }
  
  // generate form:
  c.panel_start("primary", "Change module &amp; admin password");
  c.form_start(p.uri);
  c.input_password("Old password", "oldpass", "");
  c.input_password("New password", "newpass1", "");
  c.input_password("…repeat", "newpass2", "");
  c.input_button("default", "Submit");
  c.form_end();
  c.panel_end();
  c.done();
}


/**
 * HandleCfgVehicle: configure vehicle type & identity (URL: /cfg/vehicle)
 */
void OvmsWebServer::HandleCfgVehicle(PageEntry_t& p, PageContext_t& c)
{
  std::string error, info;
  std::string vehicleid, vehicletype, vehiclename;
  
  if (c.method == "POST") {
    // process form submission:
    vehicleid = c.getvar("vehicleid");
    vehicletype = c.getvar("vehicletype");
    vehiclename = c.getvar("vehiclename");
    
    if (vehicleid.length() == 0)
      error += "<li data-input=\"vehicleid\">Vehicle ID must not be empty</li>";
    if (vehicleid.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != std::string::npos)
      error += "<li data-input=\"vehicleid\">Vehicle ID may only contain upper case letters and digits</li>";
    
    if (error == "" && StdMetrics.ms_v_type->AsString() != vehicletype) {
      MyVehicleFactory.SetVehicle(vehicletype.c_str());
      if (!MyVehicleFactory.ActiveVehicle())
        error += "<li data-input=\"vehicletype\">Cannot set vehicle type <code>" + vehicletype + "</code></li>";
      else
        info += "<li>New vehicle type <code>" + vehicletype + "</code> has been set.</li>";
    }
    
    if (error == "") {
      // success:
      MyConfig.SetParamValue("vehicle", "id", vehicleid);
      MyConfig.SetParamValue("vehicle", "type", vehicletype);
      MyConfig.SetParamValue("vehicle", "name", vehiclename);
      
      info = "<p class=\"lead\">Success!</p><ul class=\"infolist\">" + info + "</ul>";
      info += "<script>$(\"#menu\").load(\"/menu\")</script>";
      c.head(200);
      c.alert("success", info.c_str());
      OutputHome(p, c);
      c.done();
      return;
    }
    
    // output error, return to form:
    error = "<p class=\"lead\">Error!</p><ul class=\"errorlist\">" + error + "</ul>";
    c.head(400);
    c.alert("danger", error.c_str());
  }
  else {
    // read configuration:
    vehicleid = MyConfig.GetParamValue("vehicle", "id");
    vehicletype = MyConfig.GetParamValue("vehicle", "type");
    vehiclename = MyConfig.GetParamValue("vehicle", "name");
    c.head(200);
  }
  
  // generate form:
  c.panel_start("primary", "Vehicle configuration");
  c.form_start(p.uri);
  c.input_select_start("Vehicle type", "vehicletype");
  for (OvmsVehicleFactory::map_vehicle_t::iterator k=MyVehicleFactory.m_vmap.begin(); k!=MyVehicleFactory.m_vmap.end(); ++k)
    c.input_select_option(k->second.name, k->first, (vehicletype == k->first));
  c.input_select_end();
  c.input_text("Vehicle ID", "vehicleid", vehicleid.c_str(), "Use upper case letters and/or digits",
    "<p>Note: this is also the <strong>vehicle account ID</strong> for server connections.</p>");
  c.input_text("Vehicle name", "vehiclename", vehiclename.c_str(), "optional, the name of your car");
  c.input_button("default", "Save");
  c.form_end();
  c.panel_end();
  c.done();
}


/**
 * HandleCfgModem: configure APN & modem features (URL /cfg/modem)
 */
void OvmsWebServer::HandleCfgModem(PageEntry_t& p, PageContext_t& c)
{
  std::string apn, apn_user, apn_pass;
  bool enable_gps, enable_gpstime, enable_net, enable_sms;
  
  if (c.method == "POST") {
    // process form submission:
    apn = c.getvar("apn");
    apn_user = c.getvar("apn_user");
    apn_pass = c.getvar("apn_pass");
    enable_net = (c.getvar("enable_net") == "yes");
    enable_sms = (c.getvar("enable_sms") == "yes");
    enable_gps = (c.getvar("enable_gps") == "yes");
    enable_gpstime = (c.getvar("enable_gpstime") == "yes");
    
    MyConfig.SetParamValue("modem", "apn", apn);
    MyConfig.SetParamValue("modem", "apn.user", apn_user);
    MyConfig.SetParamValue("modem", "apn.password", apn_pass);
    MyConfig.SetParamValueBool("modem", "enable.net", enable_net);
    MyConfig.SetParamValueBool("modem", "enable.sms", enable_sms);
    MyConfig.SetParamValueBool("modem", "enable.gps", enable_gps);
    MyConfig.SetParamValueBool("modem", "enable.gpstime", enable_gpstime);
    
    c.head(200);
    c.alert("success", "<p class=\"lead\">Modem configured.</p>");
    OutputHome(p, c);
    c.done();
    return;
  }

  // read configuration:
  apn = MyConfig.GetParamValue("modem", "apn");
  apn_user = MyConfig.GetParamValue("modem", "apn.user");
  apn_pass = MyConfig.GetParamValue("modem", "apn.password");
  enable_net = MyConfig.GetParamValueBool("modem", "enable.net", true);
  enable_sms = MyConfig.GetParamValueBool("modem", "enable.sms", true);
  enable_gps = MyConfig.GetParamValueBool("modem", "enable.gps", false);
  enable_gpstime = MyConfig.GetParamValueBool("modem", "enable.gpstime", false);
  
  // generate form:
  c.head(200);
  c.panel_start("primary", "Modem configuration");
  c.form_start(p.uri);

  c.fieldset_start("Internet");
  c.input_checkbox("Enable IP networking", "enable_net", enable_net);
  c.input_text("APN", "apn", apn.c_str(), NULL,
    "<p>For Hologram, use APN <code>hologram</code> with empty username &amp; password</p>");
  c.input_text("…username", "apn_user", apn_user.c_str());
  c.input_text("…password", "apn_pass", apn_pass.c_str());
  c.fieldset_end();
  
  c.fieldset_start("Features");
  c.input_checkbox("Enable SMS", "enable_sms", enable_sms);
  c.input_checkbox("Enable GPS", "enable_gps", enable_gps);
  c.input_checkbox("Use GPS time", "enable_gpstime", enable_gpstime,
    "<p>Note: GPS &amp; GPS time support can be left disabled, vehicles will activate them as needed</p>");
  c.fieldset_end();
  
  c.hr();
  c.input_button("default", "Save");
  c.form_end();
  c.panel_end();
  c.done();
}


/**
 * HandleCfgServerV2: configure server v2 connection (URL /cfg/server/v2)
 */
void OvmsWebServer::HandleCfgServerV2(PageEntry_t& p, PageContext_t& c)
{
  std::string error;
  std::string server, password, port;
  std::string updatetime_connected, updatetime_idle;
  
  if (c.method == "POST") {
    // process form submission:
    server = c.getvar("server");
    password = c.getvar("password");
    port = c.getvar("port");
    updatetime_connected = c.getvar("updatetime_connected");
    updatetime_idle = c.getvar("updatetime_idle");
    
    // validate:
    if (port != "") {
      if (port.find_first_not_of("0123456789") != std::string::npos
          || atoi(port.c_str()) < 0 || atoi(port.c_str()) > 65535) {
        error += "<li data-input=\"port\">Port must be an integer value in the range 0…65535</li>";
      }
    }
    if (updatetime_connected != "") {
      if (atoi(updatetime_connected.c_str()) < 1) {
        error += "<li data-input=\"updatetime_connected\">Update interval (connected) must be at least 1 second</li>";
      }
    }
    if (updatetime_idle != "") {
      if (atoi(updatetime_idle.c_str()) < 1) {
        error += "<li data-input=\"updatetime_idle\">Update interval (idle) must be at least 1 second</li>";
      }
    }
    
    if (error == "") {
      // success:
      MyConfig.SetParamValue("server.v2", "server", server);
      if (password != "")
        MyConfig.SetParamValue("server.v2", "password", password);
      MyConfig.SetParamValue("server.v2", "port", port);
      MyConfig.SetParamValue("server.v2", "updatetime.connected", updatetime_connected);
      MyConfig.SetParamValue("server.v2", "updatetime.idle", updatetime_idle);
      
      c.head(200);
      c.alert("success", "<p class=\"lead\">Server V2 (MP) connection configured.</p>");
      OutputHome(p, c);
      c.done();
      return;
    }
    
    // output error, return to form:
    error = "<p class=\"lead\">Error!</p><ul class=\"errorlist\">" + error + "</ul>";
    c.head(400);
    c.alert("danger", error.c_str());
  }
  else {
    // read configuration:
    server = MyConfig.GetParamValue("server.v2", "server");
    password = MyConfig.GetParamValue("server.v2", "password");
    port = MyConfig.GetParamValue("server.v2", "port");
    updatetime_connected = MyConfig.GetParamValue("server.v2", "updatetime.connected");
    updatetime_idle = MyConfig.GetParamValue("server.v2", "updatetime.idle");
    
    // generate form:
    c.head(200);
  }

  c.panel_start("primary", "Server V2 (MP) configuration");
  c.form_start(p.uri);
  
  c.input_text("Host", "server", server.c_str(), "Enter host name or IP address",
    "<p>Public OVMS V2 servers:</p>"
    "<ul>"
      "<li><code>api.openvehicles.com</code> <a href=\"https://www.openvehicles.com/user/register\" target=\"_blank\">Registration</a></li>"
      "<li><code>ovms.dexters-web.de</code> <a href=\"https://dexters-web.de/?action=NewAccount\" target=\"_blank\">Registration</a></li>"
    "</ul>");
  c.input_text("Port", "port", port.c_str(), "optional, default: 6867");
  c.input_password("Vehicle password", "password", "", "empty = no change",
    "<p>Note: enter the password for the <strong>vehicle ID account</strong>, <em>not</em> your user account password</p>");

  c.fieldset_start("Update intervals");
  c.input_text("…connected", "updatetime_connected", updatetime_connected.c_str(),
    "optional, in seconds, default: 60");
  c.input_text("…idle", "updatetime_idle", updatetime_idle.c_str(),
    "optional, in seconds, default: 600");
  c.fieldset_end();
  
  c.hr();
  c.input_button("default", "Save");
  c.form_end();
  c.panel_end();
  c.done();
}


/**
 * HandleCfgServerV3: configure server v3 connection (URL /cfg/server/v3)
 */
void OvmsWebServer::HandleCfgServerV3(PageEntry_t& p, PageContext_t& c)
{
  std::string error;
  std::string server, user, password, port;
  std::string updatetime_connected, updatetime_idle;
  
  if (c.method == "POST") {
    // process form submission:
    server = c.getvar("server");
    user = c.getvar("user");
    password = c.getvar("password");
    port = c.getvar("port");
    updatetime_connected = c.getvar("updatetime_connected");
    updatetime_idle = c.getvar("updatetime_idle");
    
    // validate:
    if (port != "") {
      if (port.find_first_not_of("0123456789") != std::string::npos
          || atoi(port.c_str()) < 0 || atoi(port.c_str()) > 65535) {
        error += "<li data-input=\"port\">Port must be an integer value in the range 0…65535</li>";
      }
    }
    if (updatetime_connected != "") {
      if (atoi(updatetime_connected.c_str()) < 1) {
        error += "<li data-input=\"updatetime_connected\">Update interval (connected) must be at least 1 second</li>";
      }
    }
    if (updatetime_idle != "") {
      if (atoi(updatetime_idle.c_str()) < 1) {
        error += "<li data-input=\"updatetime_idle\">Update interval (idle) must be at least 1 second</li>";
      }
    }
    
    if (error == "") {
      // success:
      MyConfig.SetParamValue("server.v3", "server", server);
      MyConfig.SetParamValue("server.v3", "user", user);
      if (password != "")
        MyConfig.SetParamValue("server.v3", "password", password);
      MyConfig.SetParamValue("server.v3", "port", port);
      MyConfig.SetParamValue("server.v3", "updatetime.connected", updatetime_connected);
      MyConfig.SetParamValue("server.v3", "updatetime.idle", updatetime_idle);
      
      c.head(200);
      c.alert("success", "<p class=\"lead\">Server V3 (MQTT) connection configured.</p>");
      OutputHome(p, c);
      c.done();
      return;
    }
    
    // output error, return to form:
    error = "<p class=\"lead\">Error!</p><ul class=\"errorlist\">" + error + "</ul>";
    c.head(400);
    c.alert("danger", error.c_str());
  }
  else {
    // read configuration:
    server = MyConfig.GetParamValue("server.v3", "server");
    user = MyConfig.GetParamValue("server.v3", "user");
    password = MyConfig.GetParamValue("server.v3", "password");
    port = MyConfig.GetParamValue("server.v3", "port");
    updatetime_connected = MyConfig.GetParamValue("server.v3", "updatetime.connected");
    updatetime_idle = MyConfig.GetParamValue("server.v3", "updatetime.idle");
    
    // generate form:
    c.head(200);
  }

  c.panel_start("primary", "Server V3 (MQTT) configuration");
  c.form_start(p.uri);
  
  c.input_text("Host", "server", server.c_str(), "Enter host name or IP address",
    "<p>Public OVMS V3 servers (MQTT brokers):</p>"
    "<ul>"
      "<li><code>io.adafruit.com</code> <a href=\"https://accounts.adafruit.com/users/sign_in\" target=\"_blank\">Registration</a></li>"
      "<li><a href=\"https://github.com/mqtt/mqtt.github.io/wiki/public_brokers\" target=\"_blank\">More public MQTT brokers</a></li>"
    "</ul>");
  c.input_text("Port", "port", port.c_str(), "optional, default: 1883");
  c.input_text("Username", "port", port.c_str(), "Enter user login name");
  c.input_password("Password", "password", "", "Enter user password, empty = no change");

  c.fieldset_start("Update intervals");
  c.input_text("…connected", "updatetime_connected", updatetime_connected.c_str(),
    "optional, in seconds, default: 60");
  c.input_text("…idle", "updatetime_idle", updatetime_idle.c_str(),
    "optional, in seconds, default: 600");
  c.fieldset_end();
  
  c.hr();
  c.input_button("default", "Save");
  c.form_end();
  c.panel_end();
  c.done();
}


/**
 * HandleCfgWebServer: configure web server (URL /cfg/webserver)
 */
void OvmsWebServer::HandleCfgWebServer(PageEntry_t& p, PageContext_t& c)
{
  std::string error, warn;
  std::string docroot, auth_domain, auth_file;
  bool enable_files, enable_dirlist, auth_global;

  if (c.method == "POST") {
    // process form submission:
    docroot = c.getvar("docroot");
    auth_domain = c.getvar("auth_domain");
    auth_file = c.getvar("auth_file");
    enable_files = (c.getvar("enable_files") == "yes");
    enable_dirlist = (c.getvar("enable_dirlist") == "yes");
    auth_global = (c.getvar("auth_global") == "yes");
    
    // validate:
    if (docroot != "" && docroot[0] != '/') {
      error += "<li data-input=\"docroot\">Document root must start with '/'</li>";
    }
    if (docroot == "/" || docroot == "/store" || docroot == "/store/" || startsWith(docroot, "/store/ovms_config")) {
      warn += "<li data-input=\"docroot\">Document root <code>" + docroot + "</code> may open access to OVMS configuration files, consider using a sub directory</li>";
    }
    
    if (error == "") {
      // success:
      if (docroot == "")      MyConfig.DeleteInstance("http.server", "docroot");
      else                    MyConfig.SetParamValue("http.server", "docroot", docroot);
      if (auth_domain == "")  MyConfig.DeleteInstance("http.server", "auth.domain");
      else                    MyConfig.SetParamValue("http.server", "auth.domain", auth_domain);
      if (auth_file == "")    MyConfig.DeleteInstance("http.server", "auth.file");
      else                    MyConfig.SetParamValue("http.server", "auth.file", auth_file);
      
      MyConfig.SetParamValueBool("http.server", "enable.files", enable_files);
      MyConfig.SetParamValueBool("http.server", "enable.dirlist", enable_dirlist);
      MyConfig.SetParamValueBool("http.server", "auth.global", auth_global);
      
      c.head(200);
      c.alert("success", "<p class=\"lead\">Webserver configuration saved.</p>");
      if (warn != "") {
        warn = "<p class=\"lead\">Warning:</p><ul class=\"warnlist\">" + warn + "</ul>";
        c.alert("warning", warn.c_str());
      }
      OutputHome(p, c);
      c.done();
      return;
    }
    
    // output error, return to form:
    error = "<p class=\"lead\">Error!</p><ul class=\"errorlist\">" + error + "</ul>";
    c.head(400);
    c.alert("danger", error.c_str());
  }
  else {
    // read configuration:
    docroot = MyConfig.GetParamValue("http.server", "docroot");
    auth_domain = MyConfig.GetParamValue("http.server", "auth.domain");
    auth_file = MyConfig.GetParamValue("http.server", "auth.file");
    enable_files = MyConfig.GetParamValueBool("http.server", "enable.files", true);
    enable_dirlist = MyConfig.GetParamValueBool("http.server", "enable.dirlist", true);
    auth_global = MyConfig.GetParamValueBool("http.server", "auth.global", true);
    
    // generate form:
    c.head(200);
  }

  c.panel_start("primary", "Webserver configuration");
  c.form_start(p.uri);
  
  c.input_checkbox("Enable file access", "enable_files", enable_files);
  c.input_text("Root path", "docroot", docroot.c_str(), "Default: /sd");
  c.input_checkbox("Enable directory listings", "enable_dirlist", enable_dirlist);

  c.input_checkbox("Enable global file auth", "auth_global", auth_global,
    "<p>If enabled, file access is globally protected by the admin password (if set).</p>"
    "<p>Disabling allows public access to directories without an auth file.</p>");
  c.input_text("Directory auth file", "auth_file", auth_file.c_str(), "Default: .htaccess",
    "<p>Note: sub directories do not inherit the parent auth file.</p>");
  c.input_text("Auth domain/realm", "auth_domain", auth_domain.c_str(), "Default: ovms");
  
  c.input_button("default", "Save");
  c.form_end();
  c.panel_end();
  c.done();
}


/**
 * HandleCfgWifi: configure wifi networks (URL /cfg/wifi)
 */

void OvmsWebServer::HandleCfgWifi(PageEntry_t& p, PageContext_t& c)
{
  if (c.method == "POST") {
    std::string warn;
    
    // process form submission:
    UpdateWifiTable(p, c, "ap", "wifi.ap", warn);
    UpdateWifiTable(p, c, "cl", "wifi.ssid", warn);
    
    c.head(200);
    c.alert("success", "<p class=\"lead\">Wifi configuration saved.</p>");
    if (warn != "") {
      warn = "<p class=\"lead\">Warning:</p><ul class=\"warnlist\">" + warn + "</ul>";
      c.alert("warning", warn.c_str());
    }
    OutputHome(p, c);
    c.done();
    return;
  }

  // generate form:
  c.head(200);
  c.panel_start("primary", "Wifi configuration");
  c.printf(
    "<form method=\"post\" action=\"%s\" target=\"#main\">"
    , _attr(p.uri));
  
  c.fieldset_start("Access point networks");
  OutputWifiTable(p, c, "ap", "wifi.ap");
  c.fieldset_end();

  c.fieldset_start("Wifi client networks");
  OutputWifiTable(p, c, "cl", "wifi.ssid");
  c.fieldset_end();
  
  c.printf(
    "<hr>"
    "<button type=\"submit\" class=\"btn btn-default center-block\">Save</button>"
    "</form>"
    "<script>"
    "function delRow(el){"
      "$(el).parent().parent().remove();"
    "}"
    "function addRow(el){"
      "var counter = $(el).parents('table').first().prev();"
      "var pfx = counter.attr(\"name\");"
      "var nr = Number(counter.val()) + 1;"
      "var row = $('\\"
          "<tr>\\"
            "<td><button type=\"button\" class=\"btn btn-danger\" onclick=\"delRow(this)\"><strong>✖</strong></button></td>\\"
            "<td><input type=\"text\" class=\"form-control\" name=\"' + pfx + '_ssid_' + nr + '\" placeholder=\"SSID\"></td>\\"
            "<td><input type=\"password\" class=\"form-control\" name=\"' + pfx + '_pass_' + nr + '\" placeholder=\"Passphrase\"></td>\\"
          "</tr>');"
      "$(el).parent().parent().before(row).prev().find(\"input\").first().focus();"
      "counter.val(nr);"
    "}"
    "</script>");
  
  c.panel_end();
  c.done();
}

void OvmsWebServer::OutputWifiTable(PageEntry_t& p, PageContext_t& c, const std::string prefix, const std::string paramname)
{
  OvmsConfigParam* param = MyConfig.CachedParam(paramname);
  
  c.printf(
    "<div class=\"table-responsive\">"
      "<input type=\"hidden\" name=\"%s\" value=\"%d\">"
      "<table class=\"table\">"
        "<thead>"
          "<tr>"
            "<th width=\"10%\"></th>"
            "<th width=\"45%\">SSID</th>"
            "<th width=\"45%\">Passphrase</th>"
          "</tr>"
        "</thead>"
        "<tbody>"
    , _attr(prefix), param->m_map.size());
  
  int pos = 0;
  for (auto const& kv : param->m_map) {
    pos++;
    c.printf(
          "<tr>"
            "<td><button type=\"button\" class=\"btn btn-danger\" onclick=\"delRow(this)\"><strong>✖</strong></button></td>"
            "<td><input type=\"text\" class=\"form-control\" name=\"%s_ssid_%d\" value=\"%s\"></td>"
            "<td><input type=\"password\" class=\"form-control\" name=\"%s_pass_%d\" placeholder=\"no change\"></td>"
          "</tr>"
      , _attr(prefix), pos, _attr(kv.first)
      , _attr(prefix), pos);
  }
  
  c.print(
          "<tr>"
            "<td><button type=\"button\" class=\"btn btn-success\" onclick=\"addRow(this)\"><strong>✚</strong></button></td>"
            "<td></td>"
            "<td></td>"
          "</tr>"
        "</tbody>"
      "</table>"
    "</div>");
}

void OvmsWebServer::UpdateWifiTable(PageEntry_t& p, PageContext_t& c, const std::string prefix, const std::string paramname, std::string& warn)
{
  OvmsConfigParam* param = MyConfig.CachedParam(paramname);
  int i, max;
  std::string ssid, pass;
  char buf[50];
  ConfigParamMap newmap;
  
  max = atoi(c.getvar(prefix.c_str()).c_str());
  
  for (i = 1; i <= max; i++) {
    sprintf(buf, "%s_ssid_%d", prefix.c_str(), i);
    ssid = c.getvar(buf);
    if (ssid == "")
      continue;
    sprintf(buf, "%s_pass_%d", prefix.c_str(), i);
    pass = c.getvar(buf);
    if (pass == "")
      pass = param->GetValue(ssid);
    if (pass == "")
      warn += "<li>SSID <code>" + ssid + "</code> has no password</li>";
    newmap[ssid] = pass;
  }
  
  param->m_map.clear();
  param->m_map = std::move(newmap);
  param->Save();
}
