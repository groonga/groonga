#!/usr/bin/env ruby

require "optparse"

require "arrow"
require "faker"

class Generator
  def initialize
    @n_records = 2048
    @record_batch_size = 1024
    @fields = []
    @output = "data.arrow"
  end

  def run(args)
    parse_arguments!(args)
    generate
    true
  end

  private
  def parse_arguments!(args)
    parser = OptionParser.new
    parser.on("--n-records=N", Integer,
              "Generate N records",
              "(#{@n_records})") do |n|
      @n_records = n
    end
    parser.on("--record-batch-size=N", Integer,
              "Use N as record batch size",
              "(#{@record_batch_size})") do |n|
      @record_batch_size = n
    end
    parser.on("--field=NAME,TYPE", Array,
              "Add a field that names NAME as TYPE type") do |name, type|
      @fields << {
        name: name,
        type: type,
      }
    end
    parser.on("--output=PATH",
              "Output to PATH",
              "- means the standard output",
              "(#{@output})") do |path|
      @output = path
    end
    parser.parse!
  end

  def open_output(&block)
    if @output == "-"
      raw_output = Gio::UnixOutputStream.new($stdout.fileno, false)
      begin
        Arrow::GIOOutputStream.open(raw_output, &block)
      ensure
        raw_output.close
      end
    else
      Arrow::FileOutputStream.open(@output, false, &block)
    end
  end

  def generate
    open_output do |output|
      schema = Arrow::Schema.new(@fields)
      Arrow::RecordBatchStreamWriter.open(output, schema) do |writer|
        n_record_batches, rest = @n_records.divmod(@record_batch_size)
        batch_sizes = [@record_batch_size] * n_record_batches
        batch_sizes << rest unless rest.zero?
        fields = schema.fields
        builder = Arrow::RecordBatchBuilder.new(schema)
        batch_sizes.each do |n_records|
          fields.each do |field|
            values = generate_values(field, n_records)
            builder[field.name].append_values(values)
          end
          writer.write_record_batch(builder.flush)
        end
      end
    end
  end

  def generate_values(field, n)
    case field.data_type
    when Arrow::StringDataType
      n.times.collect do
        Faker::String.random
      end
    when Arrow::Int8DataType
      n.times.collect do
        Faker::Number.between(from: -127, to: 128)
      end
    when Arrow::IntegerDataType
      n.times.collect do
        Faker::Number.number(digits: 10)
      end
    else
      raise "Unsupported: #{field.inspect}"
    end
  end
end

generator = Generator.new
exit(generator.run(ARGV))
