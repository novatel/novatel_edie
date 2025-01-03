<head>
<script type='text/javascript'>
function tFont(x)
    {
    switch(x)
        {
    case "error":
        return "<font color=\"red\">";
    case "warning":
        return "<font color=\"darkred\">";
    case "info":
        return "<font color=\"blue\">";
    case "supplemental":
        return "<font color=\"purple\">";
    case "note":
        return "<font color=\"mediumvioletred\">";
    default:
        return "<font>";
        }
    }
function typeFont(x)
    {
    document.write( tFont(x) );
    }
</script>
</head>
