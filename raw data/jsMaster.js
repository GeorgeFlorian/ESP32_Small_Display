$(document).ready(function () {
    $('.text_box').load('events_placeholder.html');
    $('.relay_status1').load('relay_status1.html');
    $('.relay_status2').load('relay_status2.html');
    refresh();
});

function refresh() {
    setTimeout(function () {
        $('.text_box').load('events_placeholder.html');
        $('.relay_status1').load('relay_status1.html');
        $('.relay_status2').load('relay_status2.html');
        refresh();
    }, 1000);
}

function ValidateIPaddressOnChange(input, type) {
    var ipformat = /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;
    var strtype = "";
    switch (type) {
        case "ipaddress": strtype = "IP Address"; break;
        case "gateway": strtype = "Gateway"; break;
        case "subnet": strtype = "Subnet Mask"; break;
        case "dns": strtype = "DNS"; break;
    }

    if (!input.value.match(ipformat)) {
        document.getElementById(input.name).className =
            document.getElementById(input.name).className.replace
                (/(?:^|\s)correct(?!\S)/g, '');
        document.getElementById(input.name).className += " wrong";
        input.focus();
        alert(strtype + " is invalid!");
    }
    else if (input.value != null) {
        document.getElementById(input.name).className =
            document.getElementById(input.name).className.replace
                (/(?:^|\s)wrong(?!\S)/g, '');
        document.getElementById(input.name).className += " correct";
    }
}

function ValidateIPaddress() {
    var ipformat = /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;
    var ipaddr = document.forms["simple_form"]["ipAddress"];
    var gateway = document.forms["simple_form"]["gateway"];
    var subnet = document.forms["simple_form"]["subnet"];
    var dns = document.forms["simple_form"]["dns"];
    var counter = 0;

    if (ipaddr.value.match(ipformat)) {
        ipaddr.focus();
    } else {
        alert("You have entered an invalid IP Address!");
        ipaddr.focus();
        return (false);
    }
    if (gateway.value.match(ipformat)) {
        gateway.focus();
    } else {
        window.alert("You have entered an invalid GATEWAY Address!");
        gateway.focus();
        return (false);
    }
    if (subnet.value.match(ipformat)) {
        subnet.focus();
    } else {
        window.alert("You have entered an invalid SUBNET Address!");
        subnet.focus();
        return (false);
    }
    if (dns.value.match(ipformat)) {
        dns.focus();
    } else {
        window.alert("You have entered an invalid DNS Address!");
        dns.focus();
        return (false);
    }
}