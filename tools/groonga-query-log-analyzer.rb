#!/usr/bin/env ruby

require 'English'
require 'optparse'
require 'cgi'
require 'thread'

class GroongaQueryLogAnaylzer
  def initialize
    setup_options
  end

  def run(argv=nil)
    log_paths = @option_parser.parse!(argv || ARGV)

    parser = QueryLogParser.new
    threads = []
    log_paths.each do |log_path|
      threads << Thread.new do
        File.open(log_path) do |log|
          parser.parse(log)
        end
      end
    end
    threads.each do |thread|
      thread.join
    end

    reporter = create_reporter(parser.statistics)
    reporter.apply_options(@options)
    reporter.report
  end

  private
  def setup_options
    @options = {}
    @options[:n_entries] = 10
    @options[:order] = "-elapsed"
    @options[:color] = :auto
    @options[:output] = "-"
    @options[:slow_threshold] = 0.05
    @options[:reporter] = "console"

    @option_parser = OptionParser.new do |parser|
      parser.banner += " LOG1 ..."

      parser.on("-n", "--n-entries=N",
                Integer,
                "Show top N entries",
                "(#{@options[:n_entries]})") do |n|
        @options[:n_entries] = n
      end

      available_orders = ["elapsed", "-elapsed", "start-time", "-start-time"]
      parser.on("--order=ORDER",
                available_orders,
                "Sort by ORDER",
                "available values: [#{available_orders.join(', ')}]",
                "(#{@options[:order]})") do |order|
        @options[:order] = order
      end

      color_options = [
        [:auto, :auto],
        ["-", false],
        ["no", false],
        ["false", false],
        ["+", true],
        ["yes", true],
        ["true", true],
      ]
      parser.on("--[no-]color=[auto]",
                color_options,
                "Enable color output",
                "(#{@options[:color]})") do |color|
        if color.nil?
          @options[:color] = true
        else
          @options[:color] = color
        end
      end

      parser.on("--output=PATH",
                "Output to PATH.",
                "'-' PATH means standard output.",
                "(#{@options[:output]})") do |output|
        @options[:output] = output
      end

      parser.on("--slow-threshold=THRESHOLD",
                Float,
                "Use THRESHOLD seconds to detect slow operations.",
                "(#{@options[:slow_threshold]})") do |threshold|
        @options[:slow_threshold] = threshold
      end

      available_reporters = ["console", "json"]
      parser.on("--reporter=REPORTER",
                available_reporters,
                "Reports statistics by REPORTER.",
                "available values: [#{available_reporters.join(', ')}]",
                "(#{@options[:reporter]})") do |reporter|
        @options[:reporter] = reporter
      end
    end

    def create_reporter(statistics)
      case @options[:reporter]
      when "json"
        require 'json'
        JSONQueryLogReporter.new(statistics)
      else
        ConsoleQueryLogReporter.new(statistics)
      end
    end
  end

  class Command
    class << self
      @@registered_commands = {}
      def register(name, klass)
        @@registered_commands[name] = klass
      end

      def parse(command_path)
        name, parameters_string = command_path.split(/\?/, 2)
        parameters = {}
        parameters_string.split(/&/).each do |parameter_string|
          key, value = parameter_string.split(/\=/, 2)
          parameters[key] = CGI.unescape(value)
        end
        name = name.gsub(/\A\/d\//, '')
        name, output_type = name.split(/\./, 2)
        parameters["output_type"] = output_type if output_type
        command_class = @@registered_commands[name] || self
        command_class.new(name, parameters)
      end
    end

    attr_reader :name, :parameters
    def initialize(name, parameters)
      @name = name
      @parameters = parameters
    end

    def ==(other)
      other.is_a?(self.class) and
        @name == other.name and
        @parameters == other.parameters
    end
  end

  class SelectCommand < Command
    register("select", self)

    def sortby
      @parameters["sortby"]
    end

    def scorer
      @parameters["scorer"]
    end

    def conditions
      @parameters["filter"].split(/(?:&&|&!|\|\|)/).collect do |condition|
        condition = condition.strip
        condition = condition.gsub(/\A[\s\(]*/, '')
        condition = condition.gsub(/[\s\)]*\z/, '') unless /\(/ =~ condition
        condition
      end
    end

    def output_columns
      @parameters["output_columns"]
    end
  end

  class Statistic
    attr_reader :context_id, :start_time, :raw_command
    attr_reader :steps, :elapsed, :return_code
    def initialize(context_id)
      @context_id = context_id
      @start_time = nil
      @command = nil
      @raw_command = nil
      @steps = []
      @elapsed = nil
      @return_code = 0
    end

    def start(start_time, command)
      @start_time = start_time
      @raw_command = command
    end

    def finish(elapsed, return_code)
      @elapsed = elapsed
      @return_code = return_code
    end

    def command
      @command ||= Command.parse(@raw_command)
    end

    def elapsed_in_seconds
      nano_seconds_to_seconds(@elapsed)
    end

    def end_time
      @start_time + elapsed_in_seconds
    end

    def each_step
      previous_elapsed = 0
      ensure_parse_command
      @steps.each_with_index do |step, i|
        relative_elapsed = step[:elapsed] - previous_elapsed
        previous_elapsed = step[:elapsed]
        parsed_step = {
          :i => i,
          :elapsed => step[:elapsed],
          :elapsed_in_seconds => nano_seconds_to_seconds(step[:elapsed]),
          :relative_elapsed => relative_elapsed,
          :relative_elapsed_in_seconds => nano_seconds_to_seconds(relative_elapsed),
          :name => step[:name],
          :context => step_context(step[:name], i),
        }
        yield parsed_step
      end
    end

    def select_command?
      command.name == "select"
    end

    private
    def nano_seconds_to_seconds(nano_seconds)
      nano_seconds / 1000.0 / 1000.0 / 1000.0
    end

    def step_context(label, i)
      case label
      when "filter"
        @select_command.conditions[i]
      when "sort"
        @select_command.sortby
      when "score"
        @select_command.scorer
      when "output"
        @select_command.output_columns
      else
        nil
      end
    end

    def ensure_parse_command
      return unless select_command?
      @select_command = SelectCommand.parse(@raw_command)
    end
  end

  class SizedStatistics < Array
    def initialize(size)
      @size = size
    end
  end

  class QueryLogParser
    attr_reader :statistics
    def initialize
      @mutex = Mutex.new
      @statistics = []
    end

    def parse(input)
      statistics = []
      current_statistics = {}
      input.each_line do |line|
        case line
        when /\A(\d{4})-(\d\d)-(\d\d) (\d\d):(\d\d):(\d\d)\.(\d+)\|(.+?)\|([>:<])/
          year, month, day, hour, minutes, seconds, micro_seconds =
            $1, $2, $3, $4, $5, $6, $7
          context_id = $8
          type = $9
          rest = $POSTMATCH.strip
          time_stamp = Time.local(year, month, day, hour, minutes, seconds,
                                  micro_seconds)
          parse_line(statistics, current_statistics,
                time_stamp, context_id, type, rest)
        end
      end
      @mutex.synchronize do
        @statistics.concat(statistics)
      end
    end

    private
    def parse_line(statistics, current_statistics,
                   time_stamp, context_id, type, rest)
      case type
      when ">"
        statistic = Statistic.new(context_id)
        statistic.start(time_stamp, rest)
        current_statistics[context_id] = statistic
      when ":"
        return unless /\A(\d+) (.+)\((\d+)\)/ =~ rest
        elapsed = $1
        name = $2
        n_records = $3.to_i
        statistic = current_statistics[context_id]
        return if statistic.nil?
        statistic.steps << {
          :elapsed => elapsed.to_i,
          :name => name,
          :n_records => n_records,
        }
      when "<"
        return unless /\A(\d+) rc=(\d+)/ =~ rest
        elapsed = $1
        return_code = $2
        statistic = current_statistics.delete(context_id)
        return if statistic.nil?
        statistic.finish(elapsed.to_i, return_code.to_i)
        statistics << statistic
      end
    end
  end

  class QueryLogReporter
    include Enumerable

    attr_reader :output
    attr_accessor :n_entries, :slow_threshold
    def initialize(statistics)
      @statistics = statistics
      @order = "-elapsed"
      @n_entries = 10
      @slow_threshold = 0.05
      @output = $stdout
      @sorted_statistics = nil
    end

    def apply_options(options)
      self.order = options[:order] || @order
      self.n_entries = options[:n_entries] || @n_entries
      self.slow_threshold = options[:slow_threshold] || @slow_threshold
      self.output = options[:output] || @output
    end

    def order=(order)
      return if @order == order
      @order = order
      @sorted_statistics = nil
    end

    def output=(output)
      @output = output
      @output = $stdout if @output == "-"
    end

    def sorted_statistics
      @sorted_statistics ||= @statistics.sort_by(&sorter)
    end

    def each
      sorted_statistics.each_with_index do |statistic, i|
        break if i >= @n_entries
        yield statistic
      end
    end

    private
    def sorter
      case @order
      when "elapsed"
        lambda do |statistic|
          -statistic.elapsed
        end
      when "-elapsed"
        lambda do |statistic|
          -statistic.elapsed
        end
      when "-start-time"
        lambda do |statistic|
          -statistic.start_time
        end
      else
        lambda do |statistic|
          statistic.start_time
        end
      end
    end

    def slow?(elapsed)
      elapsed >= @slow_threshold
    end

    def setup_output
      if @output.is_a?(String)
        File.open(@output, "w") do |output|
          yield(output)
        end
      else
        yield(@output)
      end
    end
  end

  class ConsoleQueryLogReporter < QueryLogReporter
    class Color
      NAMES = ["black", "red", "green", "yellow",
               "blue", "magenta", "cyan", "white"]

      attr_reader :name
      def initialize(name, options={})
        @name = name
        @foreground = options[:foreground]
        @foreground = true if @foreground.nil?
        @intensity = options[:intensity]
        @bold = options[:bold]
        @italic = options[:italic]
        @underline = options[:underline]
      end

      def foreground?
        @foreground
      end

      def intensity?
        @intensity
      end

      def bold?
        @bold
      end

      def italic?
        @italic
      end

      def underline?
        @underline
      end

      def ==(other)
        self.class === other and
          [name, foreground?, intensity?,
           bold?, italic?, underline?] ==
          [other.name, other.foreground?, other.intensity?,
           other.bold?, other.italic?, other.underline?]
      end

      def sequence
        sequence = []
        if @name == "none"
        elsif @name == "reset"
          sequence << "0"
        else
          foreground_parameter = foreground? ? 3 : 4
          foreground_parameter += 6 if intensity?
          sequence << "#{foreground_parameter}#{NAMES.index(@name)}"
        end
        sequence << "1" if bold?
        sequence << "3" if italic?
        sequence << "4" if underline?
        sequence
      end

      def escape_sequence
        "\e[#{sequence.join(';')}m"
      end

      def +(other)
        MixColor.new([self, other])
      end
    end

    class MixColor
      attr_reader :colors
      def initialize(colors)
        @colors = colors
      end

      def sequence
        @colors.inject([]) do |result, color|
          result + color.sequence
        end
      end

      def escape_sequence
        "\e[#{sequence.join(';')}m"
      end

      def +(other)
        self.class.new([self, other])
      end

      def ==(other)
        self.class === other and colors == other.colors
      end
    end

    def initialize(statistics)
      super
      @color = :auto
      @reset_color = Color.new("reset")
      @color_schema = {
        :elapsed => {:foreground => :white, :background => :green},
        :time => {:foreground => :white, :background => :cyan},
        :slow => {:foreground => :white, :background => :red},
      }
    end

    def apply_options(options)
      super
      @color = options[:color] || @color
    end

    def report
      setup_output do |output|
        setup_color(output) do
          digit = Math.log10(n_entries).truncate + 1
          each_with_index do |statistic, i|
            output.puts "%*d) %s" % [digit, i + 1, format_heading(statistic)]
            report_parameters(output, statistic)
            report_steps(output, statistic)
          end
        end
      end
    end

    private
    def report_parameters(output, statistic)
      command = statistic.command
      output.puts "  name: <#{command.name}>"
      output.puts "  parameters:"
      command.parameters.each do |key, value|
        output.puts "    <#{key}>: <#{value}>"
      end
    end

    def report_steps(output, statistic)
      statistic.each_step do |step|
        relative_elapsed_in_seconds = step[:relative_elapsed_in_seconds]
        formatted_elapsed = "%8.8f" % relative_elapsed_in_seconds
        if slow?(relative_elapsed_in_seconds)
          formatted_elapsed = colorize(formatted_elapsed, :slow)
        end
        step_report = " %2d) %s: %s" % [step[:i] + 1,
                                         formatted_elapsed,
                                         step[:name]]
        context = step[:context]
        if context
          if slow?(relative_elapsed_in_seconds)
            context = colorize(context, :slow)
          end
          step_report << " " << context
        end
        output.puts(step_report)
      end
      output.puts
    end

    def guess_color_availability(output)
      return false unless output.tty?
      case ENV["TERM"]
      when /term(?:-color)?\z/, "screen"
        true
      else
        return true if ENV["EMACS"] == "t"
        false
      end
    end

    def setup_color(output)
      color = @color
      @color = guess_color_availability(output) if @color == :auto
      yield
    ensure
      @color = color
    end

    def format_heading(statistic)
      formatted_elapsed = colorize("%8.8f" % statistic.elapsed_in_seconds,
                                   :elapsed)
      "[%s-%s (%s)](%d): %s" % [format_time(statistic.start_time),
                                format_time(statistic.end_time),
                                formatted_elapsed,
                                statistic.return_code,
                                statistic.raw_command]
    end

    def format_time(time)
      colorize(time.strftime("%Y-%m-%d %H:%M:%S.%u"), :time)
    end

    def colorize(text, schema_name)
      return text unless @color
      options = @color_schema[schema_name]
      color = Color.new("none")
      if options[:foreground]
        color += Color.new(options[:foreground].to_s, :bold => true)
      end
      if options[:background]
        color += Color.new(options[:background].to_s, :foreground => false)
      end
      "%s%s%s" % [color.escape_sequence, text, @reset_color.escape_sequence]
    end
  end

  class JSONQueryLogReporter < QueryLogReporter
    def report
      setup_output do |output|
        output.print("[")
        each_with_index do |statistic, i|
          output.print(",") if i > 0
          output.print("\n")
          output.print(format_statistic(statistic))
        end
        output.puts
        output.puts("]")
      end
    end

    private
    def format_statistic(statistic)
      data = {
        "start_time" => statistic.start_time.to_i,
        "end_time" => statistic.end_time.to_i,
        "elapsed" => statistic.elapsed_in_seconds,
        "return_code" => statistic.return_code,
        "raw_command" => statistic.raw_command,
      }
      command = statistic.command
      parameters = command.parameters.collect do |key, value|
        {"key" => key, "value" => value}
      end
      data["command"] = {
        "name" => command.name,
        "parameters" => parameters,
      }
      steps = []
      statistic.each_step do |step|
        step_data = {}
        step_data["name"] = step[:name]
        step_data["relative_elapsed"] = step[:relative_elapsed_in_seconds]
        step_data["context"] = step[:context]
        steps << step_data
      end
      data["steps"] = steps
      JSON.generate(data)
    end
  end
end

if __FILE__ == $0
  analyzer = GroongaQueryLogAnaylzer.new
  analyzer.run
end
