(function($) {
  $.lookup = function(input, options) {
    var lastq;
    var $input = $(input).attr("autocomplete", "off");
    setTimeout(lookup, options.delay);
    function lookup() {
      var q = $input.val();
      if (lastq != q) {
        $.getJSON(options.source+"?callback=?",
                  {query: q,
                   types: 'complete',
                   table: options.table,
                   column: options.column,
                   limit: options.limit,
                   output_columns: options.output},
                  function(json) { displayItems(json[1]["complete"]); });
        lastq = q;
      }
      setTimeout(lookup, options.delay);
    }
    function displayItems(items) {
      if (items && items.length > 2) {
        var results = $("<dl />");
        items.shift();
        items.shift();
        $.each(items, function(i, val) {
          results.append($("<dt />")
            .append(
              $("<span />")
              .text(val[0])
              .click(function(){
                $(".search").val($(this).text());
                $("#search").submit();
              })));
          results.append($("<dd />")
            .append($("<span />").text(val[1]))
            .append($("<span />").text(val[2]))
          );
        });
        $("#result")
        .empty()
        .append(results);
      }
    }
  }
  $.fn.lookup = function(source, options) {
    if (!source) { return; }
    options = options || {};
    options.source = source;
    options.limit = options.limit || 25;
    options.delay = options.delay || 100;
    options.table = options.table || "item";
    options.column = options.column || "kana";
    options.output = options.output || "_key,gene95_desc,edict_desc";
    this.each(function() { new $.lookup(this, options); });
    return this;
  };
})(jQuery);
